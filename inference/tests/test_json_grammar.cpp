#include "test_common.h"
#include <nlohmann/json.hpp>
#include <cstdlib>

// This test verifies: 
// 1. Direct grammar constraint limits output structure.
// 2. JSON schema is converted to grammar and constrains output similarly.
// Both tests are heuristic: we just check the produced text begins with an expected structural token and is valid JSON when schema used.

static bool isLikelyJSON(const std::string &s) {
    if (s.empty()) return false;
    auto first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return false;
    char c = s[first];
    if (c != '{' && c != '[') return false;
    try { auto j = nlohmann::json::parse(s, nullptr, false); return !j.is_discarded(); } catch(...) { return false; }
}

int main(int argc, char **argv) {
    if (argc < 2) { std::cerr << "Usage: " << argv[0] << " <model.gguf>\n"; return 64; }
    const char *model = argv[1];

    InferenceEngine engine;
    if (!load_test_model(engine, model)) { std::cerr << "[TEST] Failed to load model\n"; return 65; }

    // 1. Direct grammar test: force output to simple key-value JSON object with fixed keys.
    {
        CompletionParameters p; p.prompt = "Provide a short object:"; p.maxNewTokens = 64; p.temperature = 0.2f; p.topP = 0.9f; p.seqId = 0; p.streaming = false;
        // Simple GBNF grammar ensuring object with fields name and value (string + integer)
        p.grammar = R"GBNF(
root ::= obj
obj ::= '{' ws '"name"' ws ':' ws string ',' ws '"value"' ws ':' ws int ws '}'
string ::= '"' chars '"'
chars ::= { char }
char ::= [^"\\] | '\\"' | '\\\\'
int ::= digit { digit }
digit ::= [0-9]
ws ::= { [ \t\n\r] }
)GBNF";
        int job = engine.submitCompletionsJob(p);
        if (job < 0) { std::cerr << "[TEST] submit grammar failed\n"; return 66; }
        engine.waitForJob(job);
        if (engine.hasJobError(job)) { std::cerr << "[TEST] grammar job error: " << engine.getJobError(job) << "\n"; return 67; }
        auto r = engine.getJobResult(job);
        if (r.text.find("\"name\"") == std::string::npos || r.text.find("\"value\"") == std::string::npos) {
            std::cerr << "[TEST] grammar constraint not reflected in output: " << r.text << "\n"; return 68; }
        std::cout << "[TEST] OK grammar completion size=" << r.tokens.size() << "\n";
    }

    // 2. JSON schema test: rely on jsonSchema conversion to grammar
    {
        nlohmann::json schema = {
            {"type", "object"},
            {"properties", {
                {"title", {{"type", "string"}}},
                {"count", {{"type", "integer"}}}
            }},
            {"required", {"title", "count"}}
        };
        CompletionParameters p; p.prompt = "Generate a JSON object:"; p.maxNewTokens = 64; p.temperature = 0.3f; p.topP = 0.9f; p.seqId = 0; p.streaming = false;
        p.jsonSchema = schema.dump();
        int job = engine.submitCompletionsJob(p);
        if (job < 0) { std::cerr << "[TEST] submit schema failed\n"; return 69; }
        engine.waitForJob(job);
        if (engine.hasJobError(job)) { std::cerr << "[TEST] schema job error: " << engine.getJobError(job) << "\n"; return 70; }
        auto r = engine.getJobResult(job);
        if (!isLikelyJSON(r.text)) { std::cerr << "[TEST] schema output not valid JSON: " << r.text << "\n"; return 71; }
        auto j = nlohmann::json::parse(r.text, nullptr, false);
        if (j.is_discarded() || !j.contains("title") || !j.contains("count")) { std::cerr << "[TEST] schema fields missing\n"; return 72; }
        std::cout << "[TEST] OK jsonSchema completion size=" << r.tokens.size() << "\n";
    }

    return 0;
}
