#include "../src/agent/agent_loop.h"
#include "../src/tools/tool_registry.h"
#include "../src/tools/calculator_tool.h"
#include "../src/tools/advanced_math_tool.h"
#include "../src/tools/memory_tool.h"
#include "../src/tools/exec_tool.h"
#include "../src/tools/translate_tool.h"
#include "../src/tools/web_search_tool.h"
#include "../src/tools/datetime_tool.h"
#include "../src/environment/environment.h"
#include "../src/tools/file_tool.h"
#include "../src/tools/text_to_speech.h"
#include "../src/tools/todolist_tool.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <fstream>
#include <memory>
#include <filesystem>
#include "../src/LLMClient/json.hpp"
#include "../src/LLMClient/OllamaClient.h"
#include "../src/utils/SystemUtils.h"
using namespace std;

int main() {
    std::cout << "==================================================\n";
    std::cout << "      KHOI DONG HE THONG AI AGENT PIPELINE        \n";
    std::cout << "==================================================\n\n";

    // 1. KHỞI TẠO LLM CLIENT
    string modelName = "google/gemma-4-31b-it"; 
    OllamaClient llmClient(modelName, "https://openrouter.ai");

    // 2. THIẾT LẬP MÔI TRƯỜNG & TOOL REGISTRY
    NativeEnvironment env;
    ToolRegistry registry;

    // Đăng ký các tool vào hệ thống
    registry.registerTool(std::make_unique<CalculatorTool>());
    registry.registerTool(std::make_unique<AdvancedMathTool>());

    registry.registerTool(std::make_unique<ExecTool>(&env, ExecPolicy::AlwaysAllow()));

    registry.registerTool(makeReadFileTool(&env));
    registry.registerTool(makeWriteFileTool(&env));

    registry.registerTool(std::make_unique<WebSearchTool>());
    
    // Đăng ký Memory Tools với Database SQLite
    auto db = std::make_shared<DatabaseManager>("memory.db");
    registry.registerTool(std::make_unique<MemorySaveTool>(db));
    registry.registerTool(std::make_unique<MemorySearchTool>(db));   

    registry.registerTool(std::make_unique<TextToSpeechTool>());
    
    registry.registerTool(std::make_unique<TranslateTool>());

    // registry.registerTool(std::make_unique<TodoListTool>());
    int maxSteps = 20;
    SkillLoader skillLoader("skills");
    skillLoader.loadAll();
    
    AgentLoop agent(&llmClient, &registry, &skillLoader, maxSteps);

    // Cắm Hook để xem log trực tiếp trên Terminal lúc Agent chạy
    agent.setStepHook([](const Step& step) {
        std::cout << "  \033[90m[Hook Log] Buoc " << step.stepId 
                  << " | Tokens: " << step.tokensUsed 
                  << " | Latency: " << step.latencyMs << "ms\033[0m\n";
    });

    // 4. KÍCH HOẠT AGENT THỰC THI TASK
    std::string taskId = "agent_loop_001";
    std::string prompt = "";
    std::string imagePath = SystemUtils::selectImageFromFolder("/workspaces/AgentAI/image");
    cout << "Yeu cau AI: ";
    cin.ignore();
    getline(cin,prompt);
    cout << endl;

    std::cout << "[Main] Giao viec cho Agent: " << prompt << "\n";
    
    // Gọi hàm run(), hệ thống sẽ tự động loop theo quy trình ReAct
    Trajectory traj = agent.run(taskId, prompt, imagePath);

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
    
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }

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

    std::string filePath = outputDir + "/trajectory_" + traj.taskId + ".json";
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << std::setw(4) << trajJson << std::endl;
        outFile.close();
        std::cout << "[Main] Da luu Trajectory thanh cong tai: " << filePath << "\n";
    } else {
        std::cerr << "[LỖI] Khong the ghi file: " << filePath << "\n";
    }
    env.teardown();
    return 0;
}