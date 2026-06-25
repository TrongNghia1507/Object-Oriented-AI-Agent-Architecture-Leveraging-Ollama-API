// tests/agent_loop_test.cpp
//
// Test AgentLoop OFFLINE bằng MockLLMClient — không cần Ollama.
// 5 kịch bản:
//   1. Happy path: gọi tool → nhận Observation → Final Answer
//   2. 2 tool liên tiếp: tính rồi lưu memory
//   3. Generic repeat loop → LoopDetector dừng
//   4. Ping-pong loop → LoopDetector dừng
//   5. max_steps → agent bị dừng sau N bước
// Ngoài ra: test step_hook (Observer) và Trajectory JSON output.

#include "../agent/agent_loop.h"
#include "../tools/tool_registry.h"
#include "../tools/calculator_tool.h"
#include "../tools/advanced_math_tool.h"
#include "../tools/memory_tool.h"
#include "../tools/exec_tool.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

// ─── MockLLMClient ────────────────────────────────────────────────────────────
// Trả về response theo thứ tự được enqueue trước.
// Không cần network, không cần Ollama.
class MockLLMClient : public LLMClient {
    std::queue<std::string> replies_;
public:
    void push(const std::string& reply) { replies_.push(reply); }

    std::expected<std::string, LLMError> sendPrompt(const std::string&) override {
        return next();
    }
    std::expected<std::string, LLMError> sendMessages(const std::vector<Message>&) override {
        return next();
    }
    std::expected<std::string, LLMError> sendPromptWithImage(
        const std::string&, const std::string&) override { return next(); }
    void setTemperature(float) override {}
    void setMaxTokens(int) override {}
    std::string getModelName() const override { return "mock-model"; }

private:
    std::expected<std::string, LLMError> next() {
        if (replies_.empty()) return std::unexpected(LLMError::UnknownNetworkError);
        auto r = replies_.front(); replies_.pop();
        return r;
    }
};

// ─── Helpers ─────────────────────────────────────────────────────────────────
static ToolRegistry makeRegistry() {
    ToolRegistry reg;
    reg.registerTool(std::make_unique<CalculatorTool>());
    reg.registerTool(std::make_unique<AdvancedMathTool>());
    reg.registerTool(std::make_unique<ExecTool>(ExecPolicy::AlwaysAllow()));
    auto db = std::make_shared<DatabaseManager>();
    reg.registerTool(std::make_unique<MemorySaveTool>(db));
    reg.registerTool(std::make_unique<MemorySearchTool>(db));
    return std::move(reg);
}

static void ok(bool cond, const std::string& msg) {
    std::cout << (cond ? "[PASS] " : "[FAIL] ") << msg << "\n";
    assert(cond);
}

// ─── Test 1: Happy path ───────────────────────────────────────────────────────
static void testHappyPath() {
    std::cout << "\n=== TEST 1: Happy path (1 tool → Final Answer) ===\n";
    MockLLMClient llm;
    llm.push("Thought: Can tinh.\nAction: calculator\nArguments: (15+5)*17");
    llm.push("Final Answer: Ket qua la 340.");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 5);
    auto traj = agent.run("t1", "Tinh (15+5)*17");

    ok(traj.success, "success = true");
    ok(traj.finalAnswer == "Ket qua la 340.", "finalAnswer khop");
    ok(traj.steps.size() == 2, "2 step");
    ok(std::holds_alternative<ToolCallAction>(traj.steps[0].action), "step0 la ToolCall");
    ok(traj.steps[0].toolResult.has_value(), "step0 co tool result");
    ok(*traj.steps[0].toolResult == "340", "calculator tra ve 340");
}

// ─── Test 2: 2 tool liên tiếp ────────────────────────────────────────────────
static void testMultiTool() {
    std::cout << "\n=== TEST 2: 2 tool lien tiep (tinh → luu memory) ===\n";
    MockLLMClient llm;
    llm.push("Thought: Tinh truoc.\nAction: calculator\nArguments: 6*7");
    llm.push("Thought: Luu ket qua.\nAction: memory_save\nArguments: [math] | 6*7=42");
    llm.push("Final Answer: Da tinh 6*7=42 va luu vao tri nho.");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 10);
    auto traj = agent.run("t2", "Tinh 6*7 roi luu vao memory");

    ok(traj.success, "success = true");
    ok(traj.steps.size() == 3, "3 step");
    auto& s0 = std::get<ToolCallAction>(traj.steps[0].action);
    auto& s1 = std::get<ToolCallAction>(traj.steps[1].action);
    ok(s0.toolName == "calculator", "step0 = calculator");
    ok(s1.toolName == "memory_save", "step1 = memory_save");
}

// ─── Test 3: Generic repeat loop ─────────────────────────────────────────────
static void testGenericRepeat() {
    std::cout << "\n=== TEST 3: Generic repeat (cung action 3 lan) ===\n";
    MockLLMClient llm;
    for (int i = 0; i < 3; ++i)
        llm.push("Thought: Thu lai.\nAction: calculator\nArguments: 1+1");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 10);
    agent.setLoopThresholds(2, 3);
    auto traj = agent.run("t3", "Tinh 1+1");

    ok(!traj.success, "success = false");
    ok(traj.stoppedByLoop, "stoppedByLoop = true");
    ok(!traj.stoppedByMaxSteps, "khong phai max steps");
}

// ─── Test 4: Ping-pong loop ───────────────────────────────────────────────────
static void testPingPong() {
    std::cout << "\n=== TEST 4: Ping-pong (A-B-A-B) ===\n";
    MockLLMClient llm;
    // 4 lần: A-B-A-B
    llm.push("Thought: A.\nAction: calculator\nArguments: 1+1");
    llm.push("Thought: B.\nAction: advanced_math\nArguments: sqrt(4)");
    llm.push("Thought: A.\nAction: calculator\nArguments: 1+1");
    llm.push("Thought: B.\nAction: advanced_math\nArguments: sqrt(4)");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 10);
    auto traj = agent.run("t4", "demo ping pong");

    ok(!traj.success, "success = false");
    ok(traj.stoppedByLoop, "stoppedByLoop = true");
}

// ─── Test 5: max_steps ────────────────────────────────────────────────────────
static void testMaxSteps() {
    std::cout << "\n=== TEST 5: max_steps ===\n";
    MockLLMClient llm;
    // 3 action khác nhau (không trigger loop), không bao giờ Final Answer
    llm.push("Thought: 1.\nAction: calculator\nArguments: 1+1");
    llm.push("Thought: 2.\nAction: calculator\nArguments: 2+2");
    llm.push("Thought: 3.\nAction: calculator\nArguments: 3+3");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 3);  // max 3 bước
    auto traj = agent.run("t5", "khong bao gio xong");

    ok(!traj.success, "success = false");
    ok(traj.stoppedByMaxSteps, "stoppedByMaxSteps = true");
    ok(traj.steps.size() == 3, "3 step");
}

// ─── Test 6: step_hook (Observer) ────────────────────────────────────────────
static void testStepHook() {
    std::cout << "\n=== TEST 6: step_hook (Observer pattern) ===\n";
    MockLLMClient llm;
    llm.push("Thought: Tinh.\nAction: calculator\nArguments: 10*10");
    llm.push("Final Answer: 100.");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 5);

    // std::function + lambda (C++17) — inject từ ngoài như HarnessRunner sẽ làm
    std::vector<int> recorded;
    agent.setStepHook([&](const Step& s) { recorded.push_back(s.stepId); });

    auto traj = agent.run("t6", "Tinh 10*10");
    ok(traj.success, "success = true");
    ok(recorded.size() == 2, "hook goi 2 lan");
    ok(recorded[0] == 0 && recorded[1] == 1, "step id dung thu tu");
}

// ─── Test 7: Trajectory JSON ──────────────────────────────────────────────────
static void testTrajectoryJson() {
    std::cout << "\n=== TEST 7: Trajectory JSON (format de bai 7.1) ===\n";
    MockLLMClient llm;
    llm.push("Thought: Tinh.\nAction: calculator\nArguments: 6*7");
    llm.push("Final Answer: 42.");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 5);
    auto traj = agent.run("task_001", "Tinh 6 nhan 7");

    auto j = traj.toJson();
    ok(j["task_id"] == "task_001", "task_id OK");
    ok(j["success"] == true, "success OK");
    ok(j["steps"].size() == 2, "2 steps trong JSON");
    ok(j["steps"][0]["action"]["type"] == "tool_call", "step0 type OK");
    ok(j["steps"][0]["action"]["tool"] == "calculator", "step0 tool OK");
    ok(j["steps"][1]["action"]["type"] == "final_answer", "step1 type OK");

    std::cout << "JSON output:\n" << j.dump(2) << "\n";
}

// ─── Test 8: Tool không tồn tại → error message → model tự điều chỉnh ────────
static void testUnknownTool() {
    std::cout << "\n=== TEST 8: Tool khong ton tai → model tu dieu chinh ===\n";
    MockLLMClient llm;
    llm.push("Thought: Thu tool ao.\nAction: nonexistent_tool\nArguments: abc");
    // Model nhận Observation lỗi và tự chuyển sang tool đúng
    llm.push("Thought: Dung calculator thay the.\nAction: calculator\nArguments: 2+2");
    llm.push("Final Answer: 4.");

    auto reg = makeRegistry();
    AgentLoop agent(&llm, &reg, nullptr, 5);
    auto traj = agent.run("t8", "Tinh 2+2");

    ok(traj.success, "success = true sau khi tu sua");
    ok(traj.steps.size() == 3, "3 step");
    // step 0: tool lỗi nhưng có result (error message)
    ok(traj.steps[0].toolResult.has_value(), "step0 co result (error msg)");
    ok(traj.steps[0].toolResult->find("LOI") != std::string::npos, "result la error message");
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== AGENT LOOP OFFLINE TESTS ===\n";

    testHappyPath();
    testMultiTool();
    testGenericRepeat();
    testPingPong();
    testMaxSteps();
    testStepHook();
    testTrajectoryJson();
    testUnknownTool();

    std::cout << "\n=== TAT CA TEST PASS ===\n";
    return 0;
}
