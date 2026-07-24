#include "../agent/agent_loop.h"
#include "../tools/tool_registry.h"
#include "../tools/calculator_tool.h"
#include "../tools/advanced_math_tool.h"
#include "../tools/memory_tool.h"
#include "../tools/exec_tool.h"
#include "../tools/translate_tool.h"
#include "../tools/web_search_tool.h"
#include "../environment/environment.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <fstream>
#include "../tools/file_tool.h"
#include <memory>
#include <filesystem>
#include "../LLMClient/json.hpp"
#include "../LLMClient/OllamaClient.h"

using namespace std;

int main() {
    std::cout << "==================================================\n";
    std::cout << "      KHOI DONG HE THONG AI AGENT PIPELINE        \n";
    std::cout << "==================================================\n\n";

    // 1. KHỞI TẠO LLM CLIENT (Đã chuyển sang OpenRouter)
    // Sử dụng model miễn phí và thông minh nhất cho logic
    string modelName = "google/gemma-4-31b-it"; 
    
    OllamaClient llmClient(modelName, "https://openrouter.ai");

    // 2. THIẾT LẬP MÔI TRƯỜNG & TOOL REGISTRY
    NativeEnvironment env; // Dùng Native để test thật trên máy
    ToolRegistry registry;

    // Đăng ký các tool vào hệ thống
    registry.registerTool(std::make_unique<CalculatorTool>());
    registry.registerTool(std::make_unique<AdvancedMathTool>());
    registry.registerTool(std::make_unique<ExecTool>(&env, ExecPolicy::AlwaysAllow()));
    registry.registerTool(makeReadFileTool(&env));
    registry.registerTool(makeWriteFileTool(&env));
    
    auto db = std::make_shared<DatabaseManager>();
    registry.registerTool(std::make_unique<MemorySaveTool>(db));
    registry.registerTool(std::make_unique<MemorySearchTool>(db));

    registry.registerTool(std::make_unique<TranslateTool>());
    registry.registerTool(std::make_unique<WebSearchTool>());

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
    // Tắt để test web search & translate
    // std::string taskId = "test_pipeline_001";
    // std::string prompt = "Quan sát bức ảnh được đính kèm và ghi kết quả tên của vật trong ảnh và trạng thái hình thể sang một file object_name.txt trong thư mục results";
    // std::string imagePath = "/workspaces/AgentAI/image/ankaramessi.png";
    // std::cout << "[Main] Giao viec cho Agent: " << prompt << "\n";


    // std::string taskId = "test_pipeline_002";
    // // Prompt này yêu cầu Agent dịch sang tiếng Anh và tìm kiếm trên web theo luật đã định
    // std::string prompt = "Hãy tìm kiếm thông tin về 'Tháp Eiffel' trên mạng và cho tôi biết chiều cao của nó.";
    // std::string imagePath = ""; // Để trống vì task này không cần ảnh
    // // ======================================================

    std::string taskId = "test_pipeline_003_memory";
    
    // === THÊM MỚI: Task để test Memory Save và Memory Search ===
    std::string prompt = "Hãy làm 2 bước sau: 1. Sử dụng công cụ lưu trữ để ghi nhớ thông tin 'Mã số két sắt của tôi là 868686' với nhãn [personal_info]. 2. Sau đó, giả vờ như bạn không biết, hãy sử dụng công cụ tìm kiếm trong bộ nhớ với từ khóa 'két sắt' để lấy lại đoạn mã số đó và trả lời cho tôi.";
    std::string imagePath = ""; 
    // ===========================================================

    std::cout << "[Main] Giao viec cho Agent: " << prompt << "\n";
    
    // Gọi hàm run(), hệ thống sẽ tự động loop theo quy trình ReAct
    Trajectory traj = agent.run(taskId, prompt,imagePath);

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