#include <iostream>
#include <memory>
#include <string>
#include "../Harness/Harness_runner.h"
#include "../agent/agent_loop.h"
#include "../LLMClient/OllamaClient.h"
#include "../tools/tool_registry.h"
#include "../agent/skill_loader.h"
#include "../environment/environment.h"

#include "../tools/tool_registry.h"
#include "../tools/calculator_tool.h"
#include "../tools/advanced_math_tool.h"
#include "../tools/memory_tool.h"
#include "../tools/exec_tool.h"
#include "../tools/translate_tool.h"
#include "../tools/web_search_tool.h"
#include "../tools/text_to_speech.h"
#include "../tools/file_tool.h"
using namespace std;

int main() {
    std::cout << "==================================================\n";
    std::cout << "        KHOI DONG HE THONG BENCHMARK              \n";
    std::cout << "==================================================\n";

    string modelName = "google/gemma-4-31b-it"; 
    
    OllamaClient llmClient(modelName, "https://openrouter.ai");

    // 2. Khởi tạo Tool Registry & Skill Loader
    ToolRegistry registry;
    // Đăng ký các tool cần thiết cho agent


    // Đăng ký các tool vào hệ thống
    
    NativeEnvironment env;

    registry.registerTool(std::make_unique<CalculatorTool>());
    registry.registerTool(std::make_unique<AdvancedMathTool>());
    registry.registerTool(std::make_unique<ExecTool>(&env, ExecPolicy::AlwaysAllow()));
    registry.registerTool(makeReadFileTool(&env));
    registry.registerTool(makeWriteFileTool(&env));
    
    auto db = std::make_shared<DatabaseManager>();
    registry.registerTool(std::make_unique<MemorySaveTool>(db));
    registry.registerTool(std::make_unique<MemorySearchTool>(db));   
    registry.registerTool(std::make_unique<TextToSpeechTool>());
    registry.registerTool(std::make_unique<TranslateTool>());
    registry.registerTool(std::make_unique<WebSearchTool>());
    SkillLoader skillLoader("skills");
    skillLoader.loadAll();

    // 3. Khởi tạo AgentLoop
    int defaultMaxSteps = 15;
    AgentLoop agent(&llmClient, &registry, &skillLoader, defaultMaxSteps);

    // 4. Khởi tạo Environment
    // HarnessRunner nhận std::shared_ptr<Environment> nên mày cần dùng make_shared
    // 5. Khởi tạo HarnessRunner
    HarnessRunner runner;
    runner.setEnvironment(&env);

    // 6. Chạy Benchmark từ file JSON
    std::string taskFile = "/workspaces/AgentAI/benchmark/task.json"; 
    std::cout << "[MAIN] Bat dau load file: " << taskFile << "\n";
    
    // Hàm này sẽ tự động đọc file, reset env, tạo evaluator, chạy agent và chấm điểm!
    runner.loadTaskFromJson(taskFile, agent);

    std::cout << "\n[MAIN] Hoan thanh toan bo Benchmark!\n";
    return 0;
}