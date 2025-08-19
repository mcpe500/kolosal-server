#include "test_common.h"
#include <cstdlib>

int main(int argc, char **argv) {
    if (argc < 3) { std::cerr << "Usage: " << argv[0] << " <model.gguf> <prompt>\n"; return 64; }
    const char *model = argv[1]; std::string prompt = argv[2];

    InferenceEngine engine;
    if (!load_test_model(engine, model)) { std::cerr << "[TEST] Failed to load model\n"; return 65; }

    CompletionParameters p; p.prompt = prompt; p.maxNewTokens = 32; p.temperature = 0.7f; p.topP = 0.9f; p.streaming = true; p.seqId = 0;
    int job = engine.submitCompletionsJob(p);
    if (job < 0) { std::cerr << "[TEST] submit failed\n"; return 66; }
    // Stream partial output as it is generated
    size_t printed = 0;
    auto start = std::chrono::steady_clock::now();
    const int timeout_ms = 20000;
    while (!engine.isJobFinished(job)) {
        if (engine.hasJobError(job)) { std::cerr << "[TEST] job error: " << engine.getJobError(job) << "\n"; return 67; }
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

    // Final result and assertions
    auto r = engine.getJobResult(job);
    if (r.text.size() > printed) { std::cout << r.text.substr(printed); std::cout.flush(); }
    std::cout << "\n";
    if (r.tokens.empty()) { std::cerr << "[TEST] no tokens generated\n"; return 68; }
    std::cout << "[TEST] OK basic completion tokens=" << r.tokens.size() << " ttft=" << r.ttft << " tps=" << r.tps << "\n";
    std::cout << "[TEST] Result: " << r.text << "\n";
    return 0;
}
