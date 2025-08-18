#include "test_common.h"
#include <filesystem>
#include <unordered_map>

static bool stream_until_done(InferenceEngine &engine, int job, int timeout_ms = 20000) {
    size_t printed = 0;
    auto start = std::chrono::steady_clock::now();
    while (!engine.isJobFinished(job)) {
        if (engine.hasJobError(job)) {
            std::cerr << "[TEST] job failed: " << engine.getJobError(job) << "\n";
            return false;
        }
        auto partial = engine.getJobResult(job);
        if (partial.text.size() > printed) {
            std::cout << partial.text.substr(printed);
            std::cout.flush();
            printed = partial.text.size();
        }
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeout_ms) {
            std::cerr << "\n[TEST] Timeout waiting for streaming result" << std::endl;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    auto r = engine.getJobResult(job);
    if (r.text.size() > printed) {
        std::cout << r.text.substr(printed);
        std::cout.flush();
    }
    std::cout << "\n";
    return true;
}

int main(int argc, char **argv) {
    if (argc < 2) { std::cerr << "Usage: " << argv[0] << " <model.gguf>\n"; return 64; }
    const char *model = argv[1];

    // Use n_parallel=2 to exercise slot acquire/release across turns
    InferenceEngine engine; if (!load_test_model(engine, model, /*n_parallel=*/2)) { std::cerr << "[TEST] load fail\n"; return 65; }

    // Prepare a temp session path and logical session id
    const int session_seq_id = 42;
    const std::filesystem::path sess_path = std::filesystem::temp_directory_path() / "kolosal_test_chat_multiturn.session";
    // Clean any previous residue
    std::error_code ec; std::filesystem::remove(sess_path, ec);

    // Turn 1: ask model to remember a keyword
    const std::string keyword = "ZEBRA-ALPHA";
    ChatCompletionParameters turn1; turn1.maxNewTokens = 64; turn1.temperature = 0.0f; turn1.topP = 1.0f; turn1.streaming = true; turn1.seqId = session_seq_id; turn1.kvCacheFilePath = sess_path.string();
    turn1.messages.push_back({"system","You are a concise assistant."});
    turn1.messages.push_back({"user", std::string("Remember the keyword ") + keyword + ". Just say OK."});

    int job1 = engine.submitChatCompletionsJob(turn1);
    if (job1 < 0) { std::cerr << "[TEST] submit turn1 failed\n"; return 66; }
    if (!stream_until_done(engine, job1)) return 67;
    auto r1 = engine.getJobResult(job1);
    if (engine.hasJobError(job1) || r1.text.empty()) { std::cerr << "[TEST] turn1 failed or empty\n"; return 68; }

    // Turn 2: ask to recall the keyword exactly, using same session seqId + cache
    ChatCompletionParameters turn2; turn2.maxNewTokens = 32; turn2.temperature = 0.0f; turn2.topP = 1.0f; turn2.streaming = true; turn2.seqId = session_seq_id; turn2.kvCacheFilePath = sess_path.string();
    turn2.messages.push_back({"system","You are a concise assistant."});
    turn2.messages.push_back({"user", std::string("Remember the keyword ") + keyword + ". Just say OK."});
    turn2.messages.push_back({"assistant", r1.text});
    turn2.messages.push_back({"user","What keyword did I give you? Answer exactly with the keyword and nothing else."});

    int job2 = engine.submitChatCompletionsJob(turn2);
    if (job2 < 0) { std::cerr << "[TEST] submit turn2 failed\n"; return 69; }
    if (!stream_until_done(engine, job2)) return 70;
    auto r2 = engine.getJobResult(job2);
    if (engine.hasJobError(job2) || r2.text.empty()) { std::cerr << "[TEST] turn2 failed or empty\n"; return 71; }

    // Basic validation: ensure the second turn included the keyword
    if (r2.text.find(keyword) == std::string::npos) {
        std::cerr << "[TEST] turn2 did not contain expected keyword. Got: '" << r2.text << "'\n";
        // Non-fatal on some models, but fail test to surface behavior regressions
        return 72;
    }

    std::cout << "[TEST] OK multi-turn chat: t1_len=" << r1.text.size() << ", t2_len=" << r2.text.size() << " ttft1_ms=" << r1.ttft << " ttft2_ms=" << r2.ttft << "\n";

    // Cleanup session cache file
    std::filesystem::remove(sess_path, ec);
    return 0;
}
