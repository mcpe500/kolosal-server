#include "test_common.h"
#include <atomic>
#include <vector>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <iostream>

static bool validate_results(InferenceEngine &engine, const std::vector<int> &jobs) {
    bool ok = true;
    for (int id : jobs) {
        CompletionResult r = engine.getJobResult(id);
        if (r.tokens.empty() || r.text.empty()) {
            std::cerr << "[TEST][VALIDATE] job id=" << id << " has empty output" << std::endl;
            ok = false;
        }
        if (engine.hasJobError(id)) {
            std::cerr << "[TEST][VALIDATE] job id=" << id << " error: " << engine.getJobError(id) << std::endl;
            ok = false;
        }
    }
    return ok;
}

int main(int argc, char **argv) {
    if (argc < 2) { std::cerr << "Usage: " << argv[0] << " <model.gguf>\n"; return 64; }
    const char *model = argv[1];

    InferenceEngine engine; if (!load_test_model(engine, model, /*n_parallel=*/4)) { std::cerr << "[TEST] load fail\n"; return 65; }

    std::vector<std::pair<int,int>> job_pairs; // (idx, jobId)
    for (int i=0;i<4;i++) {
        CompletionParameters p; p.prompt = "Parallel job " + std::to_string(i) + ": Hello"; p.maxNewTokens=24; p.temperature=0.8f; p.topP=0.95f; p.streaming=true; p.seqId = i; p.kvCacheFilePath = ""; // non-persistent
        int id = engine.submitCompletionsJob(p);
        if (id<0) { std::cerr << "[TEST] submit fail for job "<<i<<"\n"; return 66; }
        job_pairs.emplace_back(i, id);
    }

    // Stream-print partial outputs until all jobs finish
    std::unordered_map<int,size_t> printed; // jobId -> chars printed
    bool all_done = false;
    while (!all_done) {
        all_done = true;
        for (auto [idx, id] : job_pairs) {
            CompletionResult r = engine.getJobResult(id);
            size_t have = printed[id];
            if (r.text.size() > have) {
                std::cout << "[OUT " << idx << "] " << r.text.substr(have) << std::flush;
                printed[id] = r.text.size();
            }
            if (!engine.isJobFinished(id)) {
                all_done = false;
            }
        }
        if (!all_done) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << std::endl;

    // Collect, validate, and print final outputs
    std::vector<int> jobs; jobs.reserve(job_pairs.size());
    for (auto [idx,id] : job_pairs) jobs.push_back(id);
    for (auto [idx, id] : job_pairs) {
        if (engine.hasJobError(id)) {
            std::cerr << "[TEST] job error idx="<<idx<<" id="<<id<<" msg="<<engine.getJobError(id)<<"\n"; return 67; }
    }
    if (!validate_results(engine, jobs)) { return 68; }

    size_t totalTokens=0; for (int id: jobs) totalTokens += engine.getJobResult(id).tokens.size();
    if (totalTokens==0) { std::cerr << "[TEST] no tokens produced across parallel jobs\n"; return 69; }

    std::cout << "\n[TEST] Final outputs:\n";
    for (auto [idx, id] : job_pairs) {
        CompletionResult r = engine.getJobResult(id);
        std::cout << "[JOB "<<idx<<"] tokens="<< r.tokens.size() << ", tps=" << r.tps << ", ttft_ms=" << r.ttft
                  << "\n" << r.text << "\n";
    }
    std::cout << "[TEST] OK parallel totalTokens="<< totalTokens <<"\n";
    return 0;
}
