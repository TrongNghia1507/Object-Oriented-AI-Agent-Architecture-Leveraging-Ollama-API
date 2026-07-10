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
#include "../Harness/environment.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <fstream>
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
// static ToolRegistry makeRegistry() {
//     ToolRegistry reg;
//     reg.registerTool(std::make_unique<CalculatorTool>());
//     reg.registerTool(std::make_unique<AdvancedMathTool>());
//     reg.registerTool(std::make_unique<ExecTool>(ExecPolicy::AlwaysAllow()));
//     auto db = std::make_shared<DatabaseManager>();
//     reg.registerTool(std::make_unique<MemorySaveTool>(db));
//     reg.registerTool(std::make_unique<MemorySearchTool>(db));
//     return std::move(reg);
// }

// static void ok(bool cond, const std::string& msg) {
//     std::cout << (cond ? "[PASS] " : "[FAIL] ") << msg << "\n";
//     assert(cond);
// }

// // ─── Test 1: Happy path ───────────────────────────────────────────────────────
// static void testHappyPath() {
//     std::cout << "\n=== TEST 1: Happy path (1 tool → Final Answer) ===\n";
//     MockLLMClient llm;
//     llm.push("Thought: Can tinh.\nAction: calculator\nArguments: (15+5)*17");
//     llm.push("Final Answer: Ket qua la 340.");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 5);
//     auto traj = agent.run("t1", "Tinh (15+5)*17");

//     ok(traj.success, "success = true");
//     ok(traj.finalAnswer == "Ket qua la 340.", "finalAnswer khop");
//     ok(traj.steps.size() == 2, "2 step");
//     ok(std::holds_alternative<ToolCallAction>(traj.steps[0].action), "step0 la ToolCall");
//     ok(traj.steps[0].toolResult.has_value(), "step0 co tool result");
//     ok(*traj.steps[0].toolResult == "340", "calculator tra ve 340");
// }

// // ─── Test 2: 2 tool liên tiếp ────────────────────────────────────────────────
// static void testMultiTool() {
//     std::cout << "\n=== TEST 2: 2 tool lien tiep (tinh → luu memory) ===\n";
//     MockLLMClient llm;
//     llm.push("Thought: Tinh truoc.\nAction: calculator\nArguments: 6*7");
//     llm.push("Thought: Luu ket qua.\nAction: memory_save\nArguments: [math] | 6*7=42");
//     llm.push("Final Answer: Da tinh 6*7=42 va luu vao tri nho.");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 10);
//     auto traj = agent.run("t2", "Tinh 6*7 roi luu vao memory");

//     ok(traj.success, "success = true");
//     ok(traj.steps.size() == 3, "3 step");
//     auto& s0 = std::get<ToolCallAction>(traj.steps[0].action);
//     auto& s1 = std::get<ToolCallAction>(traj.steps[1].action);
//     ok(s0.toolName == "calculator", "step0 = calculator");
//     ok(s1.toolName == "memory_save", "step1 = memory_save");
// }

// // ─── Test 3: Generic repeat loop ─────────────────────────────────────────────
// static void testGenericRepeat() {
//     std::cout << "\n=== TEST 3: Generic repeat (cung action 3 lan) ===\n";
//     MockLLMClient llm;
//     for (int i = 0; i < 3; ++i)
//         llm.push("Thought: Thu lai.\nAction: calculator\nArguments: 1+1");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 10);
//     agent.setLoopThresholds(2, 3);
//     auto traj = agent.run("t3", "Tinh 1+1");

//     ok(!traj.success, "success = false");
//     ok(traj.stoppedByLoop, "stoppedByLoop = true");
//     ok(!traj.stoppedByMaxSteps, "khong phai max steps");
// }

// // ─── Test 4: Ping-pong loop ───────────────────────────────────────────────────
// static void testPingPong() {
//     std::cout << "\n=== TEST 4: Ping-pong (A-B-A-B) ===\n";
//     MockLLMClient llm;
//     // 4 lần: A-B-A-B
//     llm.push("Thought: A.\nAction: calculator\nArguments: 1+1");
//     llm.push("Thought: B.\nAction: advanced_math\nArguments: sqrt(4)");
//     llm.push("Thought: A.\nAction: calculator\nArguments: 1+1");
//     llm.push("Thought: B.\nAction: advanced_math\nArguments: sqrt(4)");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 10);
//     auto traj = agent.run("t4", "demo ping pong");

//     ok(!traj.success, "success = false");
//     ok(traj.stoppedByLoop, "stoppedByLoop = true");
// }

// // ─── Test 5: max_steps ────────────────────────────────────────────────────────
// static void testMaxSteps() {
//     std::cout << "\n=== TEST 5: max_steps ===\n";
//     MockLLMClient llm;
//     // 3 action khác nhau (không trigger loop), không bao giờ Final Answer
//     llm.push("Thought: 1.\nAction: calculator\nArguments: 1+1");
//     llm.push("Thought: 2.\nAction: calculator\nArguments: 2+2");
//     llm.push("Thought: 3.\nAction: calculator\nArguments: 3+3");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 3);  // max 3 bước
//     auto traj = agent.run("t5", "khong bao gio xong");

//     ok(!traj.success, "success = false");
//     ok(traj.stoppedByMaxSteps, "stoppedByMaxSteps = true");
//     ok(traj.steps.size() == 3, "3 step");
// }

// // ─── Test 6: step_hook (Observer) ────────────────────────────────────────────
// static void testStepHook() {
//     std::cout << "\n=== TEST 6: step_hook (Observer pattern) ===\n";
//     MockLLMClient llm;
//     llm.push("Thought: Tinh.\nAction: calculator\nArguments: 10*10");
//     llm.push("Final Answer: 100.");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 5);

//     // std::function + lambda (C++17) — inject từ ngoài như HarnessRunner sẽ làm
//     std::vector<int> recorded;
//     agent.setStepHook([&](const Step& s) { recorded.push_back(s.stepId); });

//     auto traj = agent.run("t6", "Tinh 10*10");
//     ok(traj.success, "success = true");
//     ok(recorded.size() == 2, "hook goi 2 lan");
//     ok(recorded[0] == 0 && recorded[1] == 1, "step id dung thu tu");
// }

// // ─── Test 7: Trajectory JSON ──────────────────────────────────────────────────
// static void testTrajectoryJson() {
//     std::cout << "\n=== TEST 7: Trajectory JSON (format de bai 7.1) ===\n";
//     MockLLMClient llm;
//     llm.push("Thought: Tinh.\nAction: calculator\nArguments: 6*7");
//     llm.push("Final Answer: 42.");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 5);
//     auto traj = agent.run("task_001", "Tinh 6 nhan 7");

//     auto j = traj.toJson();
//     ok(j["task_id"] == "task_001", "task_id OK");
//     ok(j["success"] == true, "success OK");
//     ok(j["steps"].size() == 2, "2 steps trong JSON");
//     ok(j["steps"][0]["action"]["type"] == "tool_call", "step0 type OK");
//     ok(j["steps"][0]["action"]["tool"] == "calculator", "step0 tool OK");
//     ok(j["steps"][1]["action"]["type"] == "final_answer", "step1 type OK");

//     std::cout << "JSON output:\n" << j.dump(2) << "\n";
// }

// // ─── Test 8: Tool không tồn tại → error message → model tự điều chỉnh ────────
// static void testUnknownTool() {
//     std::cout << "\n=== TEST 8: Tool khong ton tai → model tu dieu chinh ===\n";
//     MockLLMClient llm;
//     llm.push("Thought: Thu tool ao.\nAction: nonexistent_tool\nArguments: abc");
//     // Model nhận Observation lỗi và tự chuyển sang tool đúng
//     llm.push("Thought: Dung calculator thay the.\nAction: calculator\nArguments: 2+2");
//     llm.push("Final Answer: 4.");

//     auto reg = makeRegistry();
//     AgentLoop agent(&llm, &reg, nullptr, 5);
//     auto traj = agent.run("t8", "Tinh 2+2");

//     ok(traj.success, "success = true sau khi tu sua");
//     ok(traj.steps.size() == 3, "3 step");
//     // step 0: tool lỗi nhưng có result (error message)
//     ok(traj.steps[0].toolResult.has_value(), "step0 co result (error msg)");
//     ok(traj.steps[0].toolResult->find("LOI") != std::string::npos, "result la error message");
// }

// ─── main ─────────────────────────────────────────────────────────────────────
// int main() {
//     std::cout << "=== AGENT LOOP OFFLINE TESTS ===\n";

//     testHappyPath();
//     testMultiTool();
//     testGenericRepeat();
//     testPingPong();
//     testMaxSteps();
//     testStepHook();
//     testTrajectoryJson();
//     testUnknownTool();

//     std::cout << "\n=== TAT CA TEST PASS ===\n";
//     return 0;
// }

#include <memory>
#include <filesystem>
#include "../LLMClient/json.hpp"
#include "../LLMClient/OllamaClient.h"
using namespace std;
int main() {
    std::cout << "==================================================\n";
    std::cout << "      KHOI DONG HE THONG AI AGENT PIPELINE        \n";
    std::cout << "==================================================\n\n";
    string modelName = "qwen2.5vl:3b";
    string myGistId = "865862ba7fdd9d97acd7d24ab6e07152";
    string myGistPAT = "";
    OllamaClient llmClient = OllamaClient::withGistDiscovery(modelName, myGistId, myGistPAT);
    // 2. THIẾT LẬP MÔI TRƯỜNG & TOOL REGISTRY

    NativeEnvironment env; // Dùng Native để test thật trên máy
    ToolRegistry registry;

    // Đăng ký các tool vào hệ thống
    registry.registerTool(std::make_unique<CalculatorTool>());
    registry.registerTool(std::make_unique<AdvancedMathTool>());
    registry.registerTool(std::make_unique<ExecTool>(&env,ExecPolicy::AlwaysAllow()));
    auto db = std::make_shared<DatabaseManager>();
    registry.registerTool(std::make_unique<MemorySaveTool>(db));
    registry.registerTool(std::make_unique<MemorySearchTool>(db));
    // 3. KHỞI TẠO AGENT LOOP
    int maxSteps = 10;
    // Truyền LLM, Registry, SkillLoader (để nullptr nếu chưa test skill), và Max Steps
    AgentLoop agent(&llmClient, &registry, nullptr, maxSteps);

    // [TÙY CHỌN] Cắm Hook để xem log trực tiếp trên Terminal lúc Agent chạy
    agent.setStepHook([](const Step& step) {
        std::cout << "  \033[90m[Hook Log] Buoc " << step.stepId 
                  << " | Tokens: " << step.tokensUsed 
                  << " | Latency: " << step.latencyMs << "ms\033[0m\n";
    });

    // 4. KÍCH HOẠT AGENT THỰC THI TASK
    std::string taskId = "test_pipeline_001";
    std::string prompt = "Tính phép tính 2*3 + 9*56. Sau đó BẮT BUỘC phải gọi EXEC TOOL để lưu kết quả vào file resultsEquation.txt trong thư mục results (sài exec tool). TUYỆT ĐỐI KHÔNG được xuất 'Final Answer' nếu chưa lưu file thành công.";
    
    std::cout << "[Main] Giao viec cho Agent: " << prompt << "\n";
    
    // Gọi hàm run(), hệ thống sẽ tự động loop theo quy trình ReAct
    Trajectory traj = agent.run(taskId, prompt);

    // 5. HIỂN THỊ KẾT QUẢ CƠ BẢN
    std::cout << "\n==================================================\n";
    std::cout << "[KẾT QUẢ AGENT TRẢ VỀ]\n";
    std::cout << "- Success: " << (traj.success ? "True" : "False") << "\n";
    std::cout << "- Final Answer: " << traj.finalAnswer << "\n";
    std::cout << "- Total Tokens: " << traj.totalTokens << "\n";
    std::cout << "- Total Time: " << traj.totalTimeMs << " ms\n";
    std::cout << "==================================================\n";

    // 6. XUẤT FILE TRAJECTORY.JSON RA THƯ MỤC
    std::string outputDir = "./output_trajectories";
    
    // Tự động tạo thư mục nếu chưa tồn tại (Yêu cầu C++17)
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }

    // Chuyển đổi struct Trajectory thành định dạng JSON
    nlohmann::json trajJson = nlohmann::json::object();
    trajJson["task_id"] = traj.taskId;
    trajJson["task"] = traj.task;
    trajJson["model"] = traj.model;
    trajJson["success"] = traj.success;
    trajJson["final_answer"] = traj.finalAnswer;
    trajJson["total_tokens"] = traj.totalTokens;
    trajJson["total_time_ms"] = traj.totalTimeMs;
    trajJson["stopped_by_loop"] = traj.stoppedByLoop;
    trajJson["stopped_by_max_steps"] = traj.stoppedByMaxSteps;
    
    // Đổ mảng các bước (steps) vào JSON
    trajJson["steps"] = nlohmann::json::array();
    for (const auto& step : traj.steps) {
        nlohmann::json stepJson = nlohmann::json::object();
        stepJson["step_id"] = step.stepId;
        stepJson["thought"] = step.thought;
        stepJson["tool_result"] = step.toolResult.value_or("null");
        stepJson["tokens_used"] = step.tokensUsed;
        stepJson["latency_ms"] = step.latencyMs;
        trajJson["steps"].push_back(stepJson);
    }

    // Lưu file JSON
    std::string filePath = outputDir + "/trajectory_" + traj.taskId + ".json";
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << std::setw(4) << trajJson << std::endl;
        outFile.close();
        std::cout << "[Main] Da luu Trajectory thanh cong tai: " << filePath << "\n";
    } else {
        std::cerr << "[LỖI] Khong the ghi file: " << filePath << "\n";
    }

    return 0;
}