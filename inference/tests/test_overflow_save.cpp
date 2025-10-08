#include "inference.h"
#include "test_common.h"
#include <iostream>
#include <filesystem>

// Helper to generate long text
std::string generate_text(size_t length, const std::string& pattern = "The quick brown fox jumps over the lazy dog. ") {
    std::string result;
    result.reserve(length);
    while (result.length() < length) {
        result += pattern;
    }
    return result.substr(0, length);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model.gguf>\n";
        std::cerr << "\nThis test verifies that long contexts are saved to files\n";
        return 64;
    }
    
    const char *modelPath = argv[1];
    
    std::cout << "===========================================\n";
    std::cout << "Long Context Overflow Save Test\n";
    std::cout << "Model: " << modelPath << "\n";
    std::cout << "===========================================\n\n";
    
    // Load model with small context
    InferenceEngine engine;
    LoadingParameters lp;
    lp.n_ctx = 1024;
    lp.n_keep = 256;
    lp.n_parallel = 1;
    lp.n_batch = 256;
    lp.n_ubatch = 64;
    lp.n_gpu_layers = 100;
    
    if (!engine.loadModel(modelPath, lp)) {
        std::cerr << "[FAIL] Could not load model\n";
        return 1;
    }
    
    std::cout << "Test: Submitting oversized prompt...\n";
    
    // Create a very long prompt
    CompletionParameters params;
    params.prompt = generate_text(50000, "This is a very long context that exceeds the model's context window. ");
    params.maxNewTokens = 100;
    params.temperature = 0.8f;
    
    int jobId = engine.submitCompletionsJob(params);
    
    if (jobId < 0) {
        std::cout << "[INFO] Job was rejected (expected)\n";
    } else {
        wait_for_completion(engine, jobId, 5000);
        
        if (engine.hasJobError(jobId)) {
            std::string error = engine.getJobError(jobId);
            std::cout << "[INFO] Job error: " << error << "\n";
            
            // Check if error message mentions saved file
            if (error.find("saved to:") != std::string::npos || 
                error.find("overflow_contexts") != std::string::npos) {
                std::cout << "[PASS] Long context was saved to file\n";
                
                // Check if overflow_contexts directory exists
                if (std::filesystem::exists("overflow_contexts")) {
                    std::cout << "[PASS] overflow_contexts directory created\n";
                    
                    // List files in the directory
                    int file_count = 0;
                    for (const auto& entry : std::filesystem::directory_iterator("overflow_contexts")) {
                        std::cout << "[INFO] Found saved file: " << entry.path().filename() << "\n";
                        file_count++;
                    }
                    
                    if (file_count > 0) {
                        std::cout << "[PASS] " << file_count << " overflow file(s) created\n";
                        std::cout << "\n✅ TEST PASSED\n";
                        return 0;
                    } else {
                        std::cout << "[FAIL] No files in overflow_contexts directory\n";
                        return 1;
                    }
                } else {
                    std::cout << "[FAIL] overflow_contexts directory not created\n";
                    return 1;
                }
            } else {
                std::cout << "[FAIL] Error message doesn't mention saved file\n";
                return 1;
            }
        } else {
            std::cout << "[FAIL] Job succeeded when it should have failed\n";
            return 1;
        }
    }
    
    std::cout << "\n❌ TEST FAILED\n";
    return 1;
}
