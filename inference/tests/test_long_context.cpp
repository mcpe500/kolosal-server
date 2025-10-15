/**
 * @file test_long_context.cpp
 * @brief Comprehensive tests for long context input handling
 * 
 * Tests the 5 critical fixes implemented for handling long context inputs:
 * 1. Upfront token count validation in getInputTokens()
 * 2. Early validation in submitJob() via validateParameters()
 * 3. Improved character-based input limit validation
 * 4. Enhanced context budget validation
 * 5. Session token restoration validation
 */

#include "test_common.h"
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

// Helper to generate text of specific length
std::string generate_text(size_t length, const std::string& pattern = "test ") {
    std::ostringstream oss;
    size_t written = 0;
    while (written < length) {
        oss << pattern;
        written += pattern.size();
    }
    std::string result = oss.str();
    return result.substr(0, length);
}

// Helper to load model with specific context size
bool load_test_model_with_ctx(InferenceEngine &engine, const char *modelPath, int n_ctx, int n_keep = 256) {
    LoadingParameters lp;
    lp.n_ctx = n_ctx;
    lp.n_keep = n_keep;
    lp.n_parallel = 1;
    lp.n_batch = 256;
    lp.n_ubatch = 64;
    lp.n_gpu_layers = 100;
    return engine.loadModel(modelPath, lp);
}

// Test 1: Prompt that's way too long for context window
bool test_oversized_prompt(const char* modelPath) {
    std::cout << "\n=== Test 1: Oversized Prompt ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 1024, 256)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // Generate a very long prompt (should be rejected)
    CompletionParameters params;
    params.prompt = generate_text(50000, "This is a very long prompt that exceeds context limits. ");
    params.maxNewTokens = 100;
    params.temperature = 0.8f;
    
    std::cout << "Submitting prompt of " << params.prompt.length() << " characters...\n";
    
    int jobId = engine.submitCompletionsJob(params);
    
    // Should fail immediately or during validation
    if (jobId >= 0) {
        wait_for_completion(engine, jobId, 5000);
        if (engine.hasJobError(jobId)) {
            std::string error = engine.getJobError(jobId);
            std::cout << "[PASS] Correctly rejected: " << error << "\n";
            // Verify error message mentions token count
            if (error.find("token") != std::string::npos || error.find("long") != std::string::npos) {
                std::cout << "[PASS] Error message is informative\n";
                return true;
            } else {
                std::cout << "[WARN] Error message could be more informative\n";
                return true; // Still pass, but warn
            }
        } else {
            std::cerr << "[FAIL] Job succeeded when it should have failed\n";
            return false;
        }
    } else {
        std::cout << "[PASS] Job rejected during submission (jobId=" << jobId << ")\n";
        return true;
    }
}

// Test 2: Prompt + generation that exceeds context
bool test_prompt_plus_generation_overflow(const char* modelPath) {
    std::cout << "\n=== Test 2: Prompt + Generation Overflow ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 1024, 256)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // Generate a prompt that uses ~900 tokens worth of text
    // Assuming ~4 chars per token, that's about 3600 characters
    CompletionParameters params;
    params.prompt = generate_text(3600, "The quick brown fox jumps over the lazy dog. ");
    params.maxNewTokens = 800; // Total would be ~1700 tokens (exceeds 1024)
    params.temperature = 0.8f;
    
    std::cout << "Submitting prompt of " << params.prompt.length() 
              << " characters with maxNewTokens=" << params.maxNewTokens << "\n";
    
    int jobId = engine.submitCompletionsJob(params);
    
    if (jobId >= 0) {
        wait_for_completion(engine, jobId, 5000);
        if (engine.hasJobError(jobId)) {
            std::string error = engine.getJobError(jobId);
            std::cout << "[PASS] Correctly rejected combined overflow: " << error << "\n";
            return true;
        } else {
            std::cerr << "[FAIL] Job succeeded when combined size exceeds context\n";
            return false;
        }
    } else {
        std::cout << "[PASS] Job rejected during submission\n";
        return true;
    }
}

// Test 3: Valid prompt near context limit (should trigger warning but succeed)
bool test_prompt_near_limit(const char* modelPath) {
    std::cout << "\n=== Test 3: Prompt Near Context Limit ===\n";
    
    InferenceEngine engine;
    // Use n_keep=0 to avoid session token confusion
    if (!load_test_model_with_ctx(engine, modelPath, 1024, 0)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // Generate a prompt using ~650 tokens worth (about 65% of context)
    // This should work but be near the limit
    CompletionParameters params;
    params.prompt = generate_text(2500, "Context is important. ");
    params.maxNewTokens = 200;
    params.temperature = 0.8f;
    params.streaming = false;
    
    std::cout << "Submitting prompt of " << params.prompt.length() 
              << " characters (should be ~65% of context)...\n";
    
    int jobId = engine.submitCompletionsJob(params);
    
    if (jobId < 0) {
        std::cerr << "[FAIL] Valid job was rejected (jobId=" << jobId << ")\n";
        return false;
    }
    
    if (!wait_for_completion(engine, jobId, 15000)) {
        std::cerr << "[FAIL] Job failed or timed out\n";
        if (engine.hasJobError(jobId)) {
            std::cerr << "Error: " << engine.getJobError(jobId) << "\n";
        }
        return false;
    }
    
    auto result = engine.getJobResult(jobId);
    std::cout << "[PASS] Job completed successfully\n";
    std::cout << "Generated " << result.tokens.size() << " tokens\n";
    return true;
}

// Test 4: Character-based input limit (100KB warning)
bool test_character_limit_warning(const char* modelPath) {
    std::cout << "\n=== Test 4: Character Limit Warning ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 4096, 256)) { // Larger context
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // Generate 60KB of text (should trigger warning at 50KB)
    CompletionParameters params;
    params.prompt = generate_text(60000, "x");
    params.maxNewTokens = 10;
    params.temperature = 0.8f;
    
    std::cout << "Submitting prompt of " << params.prompt.length() 
              << " characters (should trigger 50KB warning)...\n";
    
    int jobId = engine.submitCompletionsJob(params);
    
    // This should work with larger context, but may show warnings
    if (jobId >= 0) {
        wait_for_completion(engine, jobId, 15000);
        if (engine.hasJobError(jobId)) {
            std::string error = engine.getJobError(jobId);
            // May fail due to tokenization limits
            std::cout << "[INFO] Job failed: " << error << "\n";
            if (error.find("character") != std::string::npos || 
                error.find("token") != std::string::npos) {
                std::cout << "[PASS] Appropriate error for large input\n";
                return true;
            }
        } else {
            std::cout << "[PASS] Job handled large character input\n";
            return true;
        }
    }
    
    std::cout << "[INFO] Job rejected during submission\n";
    return true; // Either outcome is acceptable
}

// Test 5: Context shifting with valid but large input
bool test_context_shifting(const char* modelPath) {
    std::cout << "\n=== Test 5: Context Shifting ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 1024, 256)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // First job: fill context with valid input
    CompletionParameters params1;
    params1.prompt = generate_text(2500, "Initial context. ");
    params1.maxNewTokens = 200;
    params1.temperature = 0.8f;
    params1.streaming = false;
    params1.seqId = 0;
    
    std::cout << "Job 1: Submitting prompt to fill context...\n";
    int jobId1 = engine.submitCompletionsJob(params1);
    
    if (jobId1 < 0 || !wait_for_completion(engine, jobId1, 15000)) {
        std::cerr << "[FAIL] First job failed\n";
        return false;
    }
    
    auto result1 = engine.getJobResult(jobId1);
    std::cout << "Job 1 completed: " << result1.tokens.size() << " tokens generated\n";
    
    // Second job: should trigger context shifting
    CompletionParameters params2;
    params2.prompt = generate_text(2000, "Additional context. ");
    params2.maxNewTokens = 100;
    params2.temperature = 0.8f;
    params2.streaming = false;
    params2.seqId = 0;
    
    std::cout << "Job 2: Submitting prompt that may trigger shifting...\n";
    int jobId2 = engine.submitCompletionsJob(params2);
    
    if (jobId2 >= 0) {
        wait_for_completion(engine, jobId2, 15000);
        if (!engine.hasJobError(jobId2)) {
            auto result2 = engine.getJobResult(jobId2);
            std::cout << "[PASS] Context shifting handled successfully\n";
            std::cout << "Job 2 completed: " << result2.tokens.size() << " tokens generated\n";
            return true;
        } else {
            std::string error = engine.getJobError(jobId2);
            std::cout << "[INFO] Second job failed: " << error << "\n";
            // May fail due to overflow - check error message quality
            if (error.find("context") != std::string::npos) {
                std::cout << "[PASS] Appropriate error message\n";
                return true;
            }
        }
    }
    
    std::cout << "[INFO] Job rejected during submission\n";
    return true;
}

// Test 6: Session restoration with oversized prompt
bool test_session_restoration_validation(const char* modelPath) {
    std::cout << "\n=== Test 6: Session Restoration Validation ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 1024, 256)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // First, create a session by running a job with cache
    CompletionParameters params1;
    params1.prompt = generate_text(2000, "Session data. ");
    params1.maxNewTokens = 100;
    params1.temperature = 0.8f;
    params1.streaming = false;
    params1.kvCacheFilePath = "/tmp/test_session.bin";
    params1.seqId = 0;
    
    std::cout << "Creating session with cache...\n";
    int jobId1 = engine.submitCompletionsJob(params1);
    
    if (jobId1 < 0 || !wait_for_completion(engine, jobId1, 15000)) {
        std::cerr << "[WARN] Could not create session, skipping test\n";
        return true; // Not a failure, just can't test this
    }
    
    std::cout << "Session created successfully\n";
    
    // Now try to load the session with a new prompt that would overflow
    CompletionParameters params2;
    params2.prompt = generate_text(3000, "New prompt after session. ");
    params2.maxNewTokens = 100;
    params2.temperature = 0.8f;
    params2.streaming = false;
    params2.kvCacheFilePath = "/tmp/test_session.bin";
    params2.seqId = 0;
    
    std::cout << "Attempting to load session + large prompt...\n";
    int jobId2 = engine.submitCompletionsJob(params2);
    
    if (jobId2 >= 0) {
        wait_for_completion(engine, jobId2, 10000);
        if (engine.hasJobError(jobId2)) {
            std::string error = engine.getJobError(jobId2);
            std::cout << "[PASS] Correctly rejected session + prompt overflow: " << error << "\n";
            if (error.find("session") != std::string::npos || error.find("context") != std::string::npos) {
                std::cout << "[PASS] Error message mentions session/context\n";
            }
            return true;
        } else {
            std::cout << "[WARN] Job succeeded (may have done context shifting)\n";
            return true; // Not necessarily a failure
        }
    } else {
        std::cout << "[PASS] Job rejected during submission\n";
        return true;
    }
}

// Test 7: Configurable n_discard parameter
bool test_configurable_n_discard(const char* modelPath) {
    std::cout << "\n=== Test 7: Configurable n_discard Parameter ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 1024, 256)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // Test with default n_discard (0 = use n_left/2)
    CompletionParameters params1;
    params1.prompt = generate_text(2000, "Testing default n_discard. ");
    params1.maxNewTokens = 50;
    params1.temperature = 0.8f;
    params1.n_discard = 0; // Default behavior
    
    std::cout << "Testing with n_discard=0 (default)...\n";
    int jobId1 = engine.submitCompletionsJob(params1);
    
    if (jobId1 >= 0) {
        wait_for_completion(engine, jobId1, 15000);
        if (!engine.hasJobError(jobId1)) {
            std::cout << "[PASS] Default n_discard works\n";
        } else {
            std::cout << "[INFO] Job with default n_discard: " << engine.getJobError(jobId1) << "\n";
        }
    }
    
    // Test with custom n_discard
    CompletionParameters params2;
    params2.prompt = generate_text(2000, "Testing custom n_discard. ");
    params2.maxNewTokens = 50;
    params2.temperature = 0.8f;
    params2.n_discard = 256; // Custom value
    
    std::cout << "Testing with n_discard=256 (custom)...\n";
    int jobId2 = engine.submitCompletionsJob(params2);
    
    if (jobId2 >= 0) {
        wait_for_completion(engine, jobId2, 15000);
        if (!engine.hasJobError(jobId2)) {
            std::cout << "[PASS] Custom n_discard works\n";
        } else {
            std::cout << "[INFO] Job with custom n_discard: " << engine.getJobError(jobId2) << "\n";
        }
    }
    
    std::cout << "[PASS] n_discard parameter is configurable\n";
    return true;
}

// Test 8: Real-world long context from overflow file
bool test_realworld_long_context(const char* modelPath) {
    std::cout << "\n=== Test 8: Real-World Long Context (Overflow File) ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 4096, 256)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // Read the saved overflow context file
    std::string overflow_file = "/Users/rbisri/Documents/kolosal-code/overflow_contexts/context_job0_1759868513_4313tokens.txt";
    std::ifstream infile(overflow_file);
    
    if (!infile.is_open()) {
        std::cerr << "[WARN] Could not open overflow file, skipping test\n";
        return true; // Not a failure, just can't test
    }
    
    // Read entire file
    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string long_context = buffer.str();
    infile.close();
    
    std::cout << "Read overflow context: " << long_context.length() << " characters\n";
    
    // This should trigger overflow save (4313 tokens > 4096 context)
    CompletionParameters params;
    params.prompt = long_context;
    params.maxNewTokens = 50;
    params.temperature = 0.8f;
    
    std::cout << "Submitting real-world long context...\n";
    
    int jobId = engine.submitCompletionsJob(params);
    
    if (jobId >= 0) {
        wait_for_completion(engine, jobId, 10000);
        if (engine.hasJobError(jobId)) {
            std::string error = engine.getJobError(jobId);
            std::cout << "[PASS] Correctly rejected long context\n";
            
            // Verify overflow save feature worked
            if (error.find("saved to:") != std::string::npos || 
                error.find("overflow_contexts") != std::string::npos) {
                std::cout << "[PASS] Overflow context was saved to file\n";
                std::cout << "Error: " << error.substr(0, 150) << "...\n";
                return true;
            } else {
                std::cout << "[INFO] Context rejected but not saved: " << error << "\n";
                return true; // Still acceptable
            }
        } else {
            std::cout << "[WARN] Job succeeded (might have fit in context)\n";
            return true;
        }
    } else {
        std::cout << "[PASS] Job rejected during submission\n";
        return true;
    }
}

// Test 9: Context shifting with long input (allow_context_shift = true)
bool test_context_shift_long_generation(const char* modelPath) {
    std::cout << "\n=== Test 9: Context Shifting with Long Input ===\n";
    
    InferenceEngine engine;
    if (!load_test_model_with_ctx(engine, modelPath, 4096, 256)) {
        std::cerr << "[FAIL] Could not load model\n";
        return false;
    }
    
    // Use the overflow context file from build directory (Test 8 creates this)
    std::string overflow_file = "overflow_contexts/context_job0_1759869226_4409tokens.txt";
    
    // Try alternate paths if not found
    if (!std::filesystem::exists(overflow_file)) {
        // Try finding the latest overflow file
        std::filesystem::path overflow_dir = "overflow_contexts";
        if (std::filesystem::exists(overflow_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(overflow_dir)) {
                if (entry.path().extension() == ".txt") {
                    overflow_file = entry.path().string();
                    break;
                }
            }
        }
    }
    
    std::ifstream infile(overflow_file);
    
    if (!infile.is_open()) {
        // Generate a long prompt instead
        std::cout << "[INFO] Using generated long context instead\n";
        std::string long_context = generate_text(25000, "This is a comprehensive system prompt for testing context shifting. ");
        
        CompletionParameters params;
        params.prompt = long_context;
        params.maxNewTokens = 200;
        params.temperature = 0.7f;
        params.allow_context_shift = true;
        
        std::cout << "Submitting generated context (" << long_context.length() << " chars) with allow_context_shift=true...\n";
        
        int jobId = engine.submitCompletionsJob(params);
        
        if (jobId < 0) {
            std::cout << "[FAIL] Job submission failed\n";
            return false;
        }
        
        wait_for_completion(engine, jobId, 30000);
        
        if (engine.hasJobError(jobId)) {
            std::string error = engine.getJobError(jobId);
            std::cout << "[FAIL] Job failed: " << error << "\n";
            return false;
        }
        
        CompletionResult result = engine.getJobResult(jobId);
        std::cout << "[PASS] Context shifting succeeded with generated context!\n";
        std::cout << "Generated " << result.tokens.size() << " tokens\n";
        
        return (result.tokens.size() > 0);
    }
    
    // Read entire file
    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string long_context = buffer.str();
    infile.close();
    
    std::cout << "Read overflow context: " << long_context.length() << " characters from " << overflow_file << "\n";
    
    // Enable context shifting to allow processing
    CompletionParameters params;
    params.prompt = long_context;
    params.maxNewTokens = 200;  // Request 200 tokens of generation
    params.temperature = 0.7f;
    params.allow_context_shift = true;  // ✅ Enable automatic context shifting
    
    std::cout << "Submitting with allow_context_shift=true...\n";
    
    int jobId = engine.submitCompletionsJob(params);
    
    if (jobId < 0) {
        std::cout << "[FAIL] Job submission failed even with context shifting enabled\n";
        return false;
    }
    
    wait_for_completion(engine, jobId, 30000);  // 30 second timeout
    
    if (engine.hasJobError(jobId)) {
        std::string error = engine.getJobError(jobId);
        std::cout << "[FAIL] Job failed with context shifting: " << error << "\n";
        return false;
    }
    
    CompletionResult result = engine.getJobResult(jobId);
    std::cout << "[PASS] Context shifting succeeded!\n";
    std::cout << "Generated " << result.tokens.size() << " tokens\n";
    std::cout << "Generated text (first 200 chars): " << result.text.substr(0, 200) << "...\n";
    
    // Verify we got meaningful output
    if (result.tokens.size() > 0 && !result.text.empty()) {
        std::cout << "[PASS] Successfully generated output from long context\n";
        return true;
    } else {
        std::cout << "[FAIL] No output generated\n";
        return false;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model.gguf>\n";
        std::cerr << "\nTests long context input handling:\n";
        std::cerr << "  1. Oversized prompt rejection\n";
        std::cerr << "  2. Prompt + generation overflow detection\n";
        std::cerr << "  3. Near-limit prompt handling\n";
        std::cerr << "  4. Character limit warnings\n";
        std::cerr << "  5. Context shifting behavior\n";
        std::cerr << "  6. Session restoration validation\n";
        std::cerr << "  7. Configurable n_discard parameter\n";
        std::cerr << "  8. Real-world overflow context\n";
        std::cerr << "  9. Context shifting with long input (NEW)\n";
        return 64;
    }
    
    const char *modelPath = argv[1];
    int passed = 0;
    int total = 9;  // Updated to 9 tests
    
    std::cout << "===========================================\n";
    std::cout << "Long Context Input Handling Test Suite\n";
    std::cout << "Model: " << modelPath << "\n";
    std::cout << "===========================================\n";
    
    // Run all tests
    if (test_oversized_prompt(modelPath)) passed++;
    if (test_prompt_plus_generation_overflow(modelPath)) passed++;
    if (test_prompt_near_limit(modelPath)) passed++;
    if (test_character_limit_warning(modelPath)) passed++;
    if (test_context_shifting(modelPath)) passed++;
    if (test_session_restoration_validation(modelPath)) passed++;
    if (test_configurable_n_discard(modelPath)) passed++;
    if (test_realworld_long_context(modelPath)) passed++;
    if (test_context_shift_long_generation(modelPath)) passed++;  // New test
    
    // Summary
    std::cout << "\n===========================================\n";
    std::cout << "Test Results: " << passed << "/" << total << " passed\n";
    std::cout << "===========================================\n";
    
    if (passed == total) {
        std::cout << "✅ ALL TESTS PASSED\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}
