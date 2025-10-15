#include "inference.h"
#include "test_common.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// Helper function to generate text of specific length
std::string generate_text(size_t length, const std::string& pattern = "The quick brown fox jumps over the lazy dog. ") {
    std::string result;
    result.reserve(length);
    while (result.length() < length) {
        result += pattern;
    }
    return result.substr(0, length);
}

// Helper function to load model with specific context and n_keep
bool load_test_model_with_ctx(InferenceEngine &engine, const char *modelPath, int n_ctx, int n_keep) {
    LoadingParameters lp;
    lp.n_ctx = n_ctx;
    lp.n_keep = n_keep;
    lp.n_parallel = 1;
    lp.n_batch = 512;
    lp.n_ubatch = 128;
    lp.n_gpu_layers = 100;
    return engine.loadModel(modelPath, lp);
}

// Test TTFT with specific configuration
struct TTFTResult {
    bool success;
    float ttft_ms;
    int prompt_tokens;
    int context_size;
    int n_keep;
    std::string error;
};

TTFTResult test_ttft(const char* modelPath, int n_ctx, int n_keep, int prompt_chars) {
    TTFTResult result;
    result.context_size = n_ctx;
    result.n_keep = n_keep;
    result.success = false;
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, n_ctx, n_keep)) {
        result.error = "Failed to load model";
        return result;
    }
    
    // Generate prompt of specified size
    CompletionParameters params;
    params.prompt = generate_text(prompt_chars, "This is a test prompt for measuring time to first token performance. ");
    params.maxNewTokens = 50;  // Generate fewer tokens to leave room
    params.temperature = 0.8f;
    params.streaming = false;
    
    int jobId = engine.submitCompletionsJob(params);
    
    if (jobId < 0) {
        result.error = "Job submission failed";
        return result;
    }
    
    // Wait for completion
    if (!wait_for_completion(engine, jobId, 60000)) {
        if (engine.hasJobError(jobId)) {
            result.error = engine.getJobError(jobId);
        } else {
            result.error = "Timeout waiting for completion";
        }
        return result;
    }
    
    // Get the result which includes TTFT
    auto job_result = engine.getJobResult(jobId);
    result.ttft_ms = job_result.ttft;
    result.prompt_tokens = job_result.prompt_token_count;
    
    result.success = true;
    return result;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model.gguf>\n";
        std::cerr << "\nThis test measures Time To First Token (TTFT) performance\n";
        std::cerr << "with different context sizes and n_keep values.\n";
        return 64;
    }
    
    const char *modelPath = argv[1];
    
    std::cout << "===========================================\n";
    std::cout << "Time To First Token (TTFT) Performance Test\n";
    std::cout << "Model: " << modelPath << "\n";
    std::cout << "===========================================\n\n";
    
    // Test configurations
    struct TestConfig {
        int n_ctx;
        int n_keep;
        int prompt_chars;
        std::string description;
    };
    
    std::vector<TestConfig> configs = {
        // n_keep = 1024, varying context sizes
        // Note: With n_keep=1024, effective available context may be n_ctx/2 due to internal management
        {2048, 1024, 3500, "n_ctx=2048, n_keep=1024, ~900 token input"},
        {4096, 1024, 3500, "n_ctx=4096, n_keep=1024, ~900 token input"},
        {4096, 1024, 7500, "n_ctx=4096, n_keep=1024, ~1900 token input"},
        {8192, 1024, 3500, "n_ctx=8192, n_keep=1024, ~900 token input"},
        {8192, 1024, 7500, "n_ctx=8192, n_keep=1024, ~1900 token input"},
        {8192, 1024, 15500, "n_ctx=8192, n_keep=1024, ~3900 token input"},
    };
    
    // Header
    std::cout << std::left 
              << std::setw(50) << "Configuration" 
              << std::setw(15) << "Prompt Tokens"
              << std::setw(15) << "TTFT (ms)"
              << std::setw(10) << "Status"
              << "\n";
    std::cout << std::string(90, '-') << "\n";
    
    // Run tests
    std::vector<TTFTResult> results;
    for (const auto& config : configs) {
        std::cout << std::left << std::setw(50) << config.description << std::flush;
        
        auto result = test_ttft(modelPath, config.n_ctx, config.n_keep, config.prompt_chars);
        results.push_back(result);
        
        if (result.success) {
            std::cout << std::setw(15) << result.prompt_tokens
                      << std::setw(15) << std::fixed << std::setprecision(2) << result.ttft_ms
                      << std::setw(10) << "✅ PASS"
                      << "\n";
        } else {
            std::cout << std::setw(15) << "N/A"
                      << std::setw(15) << "N/A"
                      << std::setw(10) << "❌ FAIL"
                      << "\n";
            std::cout << "  Error: " << result.error << "\n";
        }
    }
    
    // Summary
    std::cout << "\n===========================================\n";
    std::cout << "Summary\n";
    std::cout << "===========================================\n";
    
    int passed = 0;
    for (const auto& r : results) {
        if (r.success) passed++;
    }
    
    std::cout << "Tests passed: " << passed << "/" << results.size() << "\n";
    
    if (passed > 0) {
        // Find fastest and slowest
        float min_ttft = 1e9;
        float max_ttft = 0;
        for (const auto& r : results) {
            if (r.success) {
                if (r.ttft_ms < min_ttft) min_ttft = r.ttft_ms;
                if (r.ttft_ms > max_ttft) max_ttft = r.ttft_ms;
            }
        }
        
        std::cout << "\nFastest TTFT: " << std::fixed << std::setprecision(2) << min_ttft << " ms\n";
        std::cout << "Slowest TTFT: " << std::fixed << std::setprecision(2) << max_ttft << " ms\n";
        std::cout << "Difference: " << std::fixed << std::setprecision(2) << (max_ttft - min_ttft) << " ms\n";
    }
    
    std::cout << "\n";
    
    return (passed == results.size()) ? 0 : 1;
}
