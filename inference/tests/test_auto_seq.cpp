#include "test_common.h"
#include <vector>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) { std::cerr << "Usage: " << argv[0] << " <model.gguf>\n"; return 64; }
    const char *model = argv[1];

    // Load with n_parallel=3 to allow 3 concurrent sequences
    InferenceEngine engine; if (!load_test_model(engine, model, /*n_parallel=*/3)) { std::cerr << "[TEST] load fail\n"; return 65; }

    std::vector<int> jobs;
    for (int i = 0; i < 3; ++i) {
        CompletionParameters p; p.prompt = std::string("Auto-seq job ") + std::to_string(i) + ": Hello"; p.maxNewTokens=16; p.temperature=0.7f; p.topP=0.9f; p.streaming=false; 
        p.seqId = -1; // request auto assignment
        int id = engine.submitCompletionsJob(p);
        if (id < 0) { std::cerr << "[TEST] submit fail for job "<< i <<"\n"; return 66; }
        jobs.push_back(id);
    }

    for (int id : jobs) engine.waitForJob(id);
    for (int id : jobs) if (engine.hasJobError(id)) { std::cerr << "[TEST] job error id="<<id<<" msg="<<engine.getJobError(id)<<"\n"; return 67; }

    size_t totalTokens = 0; for (int id: jobs) totalTokens += engine.getJobResult(id).tokens.size();
    if (totalTokens == 0) { std::cerr << "[TEST] no tokens produced across auto-seq jobs\n"; return 68; }
    std::cout << "[TEST] OK auto-seq totalTokens="<< totalTokens <<"\n"; return 0;
}
