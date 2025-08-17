#include "test_common.h"

int main(int argc, char **argv) {
    if (argc < 2) { std::cerr << "Usage: " << argv[0] << " <model.gguf>\n"; return 64; }
    const char *model = argv[1];
    InferenceEngine engine; if (!load_test_model(engine, model)) { std::cerr << "[TEST] load fail\n"; return 65; }

    ChatCompletionParameters chat; chat.maxNewTokens = 48; chat.temperature = 0.8f; chat.topP = 0.95f; chat.streaming = true; chat.seqId = 0;
    chat.messages.push_back({"system","You are a concise assistant."});
    chat.messages.push_back({"user","List three prime numbers greater than 50."});

    int job = engine.submitChatCompletionsJob(chat);
    if (job < 0) { std::cerr << "[TEST] submit failed\n"; return 66; }

    // Stream output: print incremental tokens as they arrive
    size_t printed = 0;
    auto start = std::chrono::steady_clock::now();
    const int timeout_ms = 20000; // safety timeout for tests
    while (!engine.isJobFinished(job)) {
        if (engine.hasJobError(job)) {
            std::cerr << "[TEST] job failed: " << engine.getJobError(job) << "\n";
            return 67;
        }
        auto partial = engine.getJobResult(job);
        if (partial.text.size() > printed) {
            std::cout << partial.text.substr(printed);
            std::cout.flush();
            printed = partial.text.size();
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > timeout_ms) {
            std::cerr << "\n[TEST] Timeout waiting for streaming result" << std::endl;
            return 67;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    // Print any final tail and newline
    auto r = engine.getJobResult(job);
    if (r.text.size() > printed) {
        std::cout << r.text.substr(printed);
        std::cout.flush();
    }
    std::cout << "\n";

    if (r.text.find("53")==std::string::npos) {
        std::cerr << "[TEST] expected prime not found in response: " << r.text << "\n"; // heuristic
        return 68;
    }
    std::cout << "[TEST] OK chat completion length=" << r.text.size() << "\n";
    std::cout << "[TEST] Result: " << r.text << "\n";
    return 0;
}
