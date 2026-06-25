// tests/llm_agent_test.cpp
//
// Demo AgentLoop đầy đủ với Ollama model thật.
// Tích hợp: OllamaClient + ToolRegistry + SkillLoader + AgentLoop.
//
// Cách chạy (từ thư mục gốc dự án):
//   export OLLAMA_MODEL="qwen2.5:0.5b"
//   export OLLAMA_BASE_URL="http://localhost:11434"
//   ./build/llm_agent_test
//
// Hoặc qua Gist tunnel:
//   export OLLAMA_MODEL="qwen2.5:7b"
//   export OLLAMA_GIST_ID="<id>"
//   export OLLAMA_GIST_PAT="<pat>"
//   ./build/llm_agent_test

#include "../agent/agent_loop.h"
#include "../LLMClient/OllamaClient.h"
#include "../tools/tool_registry.h"
#include "../tools/calculator_tool.h"
#include "../tools/advanced_math_tool.h"
#include "../tools/memory_tool.h"
#include "../tools/exec_tool.h"
#include "../tools/web_search_tool.h"
#include "../skills/skill_loader.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

namespace fs = std::filesystem;

// Lưu trajectory ra file JSON trong thư mục results/
static void saveTrajectory(const Trajectory& traj) {
    fs::create_directories("results");
    std::string path = "results/trajectory_" + traj.taskId + ".json";
    std::ofstream f(path);
    if (f.is_open()) {
        f << traj.toJson().dump(2);
        std::cout << "[AgentLoop] Trajectory saved: " << path << "\n";
    }
}

int main() {
    // ── 1. Cấu hình LLM qua biến môi trường ─────────────────────────────────
    const char* modelEnv  = std::getenv("OLLAMA_MODEL");
    const char* baseUrl   = std::getenv("OLLAMA_BASE_URL");
    const char* gistId    = std::getenv("OLLAMA_GIST_ID");
    const char* gistPat   = std::getenv("OLLAMA_GIST_PAT");

    if (!modelEnv) {
        std::cerr << "Thieu OLLAMA_MODEL. Vi du:\n"
                  << "  export OLLAMA_MODEL=\"qwen2.5:0.5b\"\n"
                  << "  export OLLAMA_BASE_URL=\"http://localhost:11434\"\n";
        return 1;
    }
    if (!baseUrl && !(gistId && gistPat)) {
        std::cerr << "Can OLLAMA_BASE_URL hoac (OLLAMA_GIST_ID + OLLAMA_GIST_PAT).\n";
        return 1;
    }

    auto llm = baseUrl
        ? std::make_unique<OllamaClient>(modelEnv, baseUrl)
        : std::make_unique<OllamaClient>(
              OllamaClient::withGistDiscovery(modelEnv, gistId, gistPat));

    llm->setTemperature(0.1f);   // thấp để model tuân thủ format tốt hơn
    llm->setMaxTokens(512);

    std::cout << "[Config] Model: " << llm->getModelName() << "\n";
    std::cout << "[Config] URL:   " << (baseUrl ? baseUrl : "Gist discovery") << "\n\n";

    // ── 2. Đăng ký Tool ──────────────────────────────────────────────────────
    ToolRegistry registry;
    registry.registerTool(std::make_unique<CalculatorTool>());
    registry.registerTool(std::make_unique<AdvancedMathTool>());
    // ExecTool: AskUser — hỏi xác nhận trước khi chạy lệnh shell
    registry.registerTool(std::make_unique<ExecTool>(ExecPolicy::AskUser()));
    registry.registerTool(std::make_unique<WebSearchTool>());
    auto db = std::make_shared<DatabaseManager>();
    registry.registerTool(std::make_unique<MemorySaveTool>(db));
    registry.registerTool(std::make_unique<MemorySearchTool>(db));

    // ── 3. Nạp Skill ─────────────────────────────────────────────────────────
    SkillLoader skillLoader("skills");
    size_t numSkills = skillLoader.loadAll();
    std::cout << "[Config] Loaded " << numSkills << " skills\n\n";

    // ── 4. Khởi tạo AgentLoop ────────────────────────────────────────────────
    AgentLoop agent(llm.get(), &registry, &skillLoader, /*maxSteps=*/8);

    // Inject step hook — in progress real-time (giống HarnessRunner sau này)
    agent.setStepHook([](const Step& s) {
        std::cout << "  [hook] step " << s.stepId
                  << " latency=" << s.latencyMs << "ms"
                  << (s.toolResult.has_value() ? " toolResult=OK" : "") << "\n";
    });

    // ── 5. Chạy các task ─────────────────────────────────────────────────────
    struct Task { std::string id, instruction; };
    std::vector<Task> tasks = {
        {"task_math_01",   "Tinh ket qua cua (12 + 8) * 5 roi cho toi biet."},
        {"task_math_02",   "Can bac hai cua 144 cong voi 2 mu 3 bang bao nhieu?"},
        {"task_memory_01", "Luu vao tri nho: ket qua bai toan chieu nay la 340. "
                           "Sau do tim lai xem tri nho co gi ve bai toan."},
    };

    for (const auto& task : tasks) {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "TASK: " << task.id << "\n";
        std::cout << task.instruction << "\n";
        std::cout << std::string(60, '-') << "\n";

        Trajectory traj = agent.run(task.id, task.instruction);

        std::cout << "\n--- KET QUA ---\n"
                  << "Success:     " << (traj.success ? "true" : "false") << "\n"
                  << "Steps:       " << traj.steps.size() << "\n"
                  << "TotalTime:   " << traj.totalTimeMs << "ms\n"
                  << "FinalAnswer: " << traj.finalAnswer << "\n";

        saveTrajectory(traj);
    }

    std::cout << "\n=== HOAN THANH " << tasks.size() << " TASK ===\n";
    return 0;
}
