#include "../src/agent/agent_loop.h"
#include "../src/tools/tool_registry.h"
#include "../src/tools/calculator_tool.h"
#include "../src/tools/advanced_math_tool.h"
#include "../src/tools/memory_tool.h"
#include "../src/tools/exec_tool.h"
#include "../src/tools/translate_tool.h"
#include "../src/tools/web_search_tool.h"
#include "../src/tools/file_tool.h"
#include "../src/tools/text_to_speech.h"
#include "../src/environment/environment.h"
#include "../src/Harness/message_queue.h" // Nhúng hàng đợi giao tiếp
#include "../src/LLMClient/json.hpp"
#include "../src/LLMClient/OllamaClient.h"
#include "../src/agent/skill_loader.h"

#include <algorithm>
#include <cctype>
#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <fstream>
#include <filesystem>
#include <thread>

using namespace std;

// ---------------------------------------------------------
// HÀM TRỌNG TÀI: LLM-as-a-Judge
// ---------------------------------------------------------
// So sánh 2 câu trả lời của 2 agent cho CÙNG một `originalQuestion`, dùng LLM
// làm trọng tài. So với bản cũ:
//   1. Có thêm ngữ cảnh `originalQuestion` để Judge phân biệt được đâu là
//      "dữ kiện cốt lõi" (nhiệm vụ có hoàn thành không) và đâu là chi tiết
//      phụ (tên file, thứ tự bước, cách hành văn) -> tránh báo XUNG DOT oan
//      khi 2 agent cùng thành công nhưng khác tên file output.
//   2. Prompt có tiêu chí rõ ràng + ví dụ minh hoạ (few-shot).
//   3. Yêu cầu Judge trả JSON có cấu trúc thay vì 1 từ TRUE/FALSE, parse an
//      toàn hơn và log được lý do (`reason`) để debug khi Judge phán sai.
bool evaluateConsensus(LLMClient& judgeClient,
                        const std::string& originalQuestion,
                        const std::string& answer1,
                        const std::string& answer2) {
    if (answer1.empty() || answer2.empty()) {
        return false;
    }

    std::string prompt =
        "Ban la mot trong tai cong tam, danh gia xem 2 AI Agent co HOAN THANH CUNG MOT "
        "NHIEM VU hay khong, khong phai danh gia xem cau chu co giong het nhau khong.\n\n"
        "Nhiem vu goc duoc giao cho ca hai agent:\n\"" + originalQuestion + "\"\n\n"
        "Cau tra loi cua Agent 1:\n\"" + answer1 + "\"\n\n"
        "Cau tra loi cua Agent 2:\n\"" + answer2 + "\"\n\n"
        "TIEU CHI DANH GIA:\n"
        "- Chi coi la MAU THUAN (conflict) neu hai cau tra loi bat dong ve KET QUA CUA "
        "NHIEM VU: mot ben noi THANH CONG con ben kia noi THAT BAI, hoac hai ben dua ra "
        "hai dap an/gia tri khac nhau va loai tru lan nhau cho CUNG mot cau hoi cot loi.\n"
        "- KHONG coi la mau thuan neu khac biet chi la: ten file output khac nhau, thu tu "
        "cac buoc thu lai, do dai cau chu, cach dien dat, thoi gian xu ly (latency), hay "
        "cac chi tiet ky thuat phu khong anh huong den viec nguoi dung co nhan duoc ket "
        "qua ho can hay khong.\n\n"
        "VI DU:\n"
        "- Agent 1: 'Da luu am thanh tai output.mp3'. Agent 2: 'Da luu am thanh tai "
        "ronaldo.mp3'. -> DONG THUAN (ca hai deu hoan thanh viec chuyen doi van ban "
        "thanh am thanh, chi khac ten file).\n"
        "- Agent 1: 'Khong the tao am thanh do loi API'. Agent 2: 'Da tao am thanh thanh "
        "cong'. -> MAU THUAN (mot ben that bai, mot ben thanh cong).\n\n"
        "Tra loi DUY NHAT bang JSON theo dung dinh dang sau, khong them chu nao khac:\n"
        "{\"verdict\": \"DONG_THUAN\" hoac \"MAU_THUAN\", \"reason\": \"ly do ngan gon\"}";

    auto result = judgeClient.sendPrompt(prompt);

    if (!result) {
        std::cerr << "[Judge LLM] Loi khi goi LLM danh gia.\n";
        return false;
    }

    std::string judgeReply = result.value();

    // Model đôi khi bọc JSON trong ```json ... ``` dù đã dặn không thêm chữ khác —
    // cắt bỏ phần fence nếu có, để parse JSON không bị lỗi.
    auto stripCodeFence = [](std::string s) {
        size_t start = s.find('{');
        size_t end = s.rfind('}');
        if (start != std::string::npos && end != std::string::npos && end > start) {
            return s.substr(start, end - start + 1);
        }
        return s;
    };
    std::string cleanReply = stripCodeFence(judgeReply);

    try {
        nlohmann::json parsed = nlohmann::json::parse(cleanReply);
        std::string verdict = parsed.value("verdict", "");
        std::string reason  = parsed.value("reason", "");

        std::transform(verdict.begin(), verdict.end(), verdict.begin(), ::toupper);

        std::cout << "[Judge LLM] Ly do: " << reason << "\n";

        return verdict.find("DONG_THUAN") != std::string::npos;
    } catch (const nlohmann::json::parse_error& e) {
        // Fallback: nếu model không tuân thủ JSON, quay lại kiểu so khớp từ khoá cũ
        // để không làm hỏng luồng chạy, nhưng log cảnh báo để bạn biết cần chỉnh prompt.
        std::cerr << "[Judge LLM] Canh bao: khong parse duoc JSON tu Judge ("
                  << e.what() << "). Raw reply: " << judgeReply << "\n";

        std::string upperReply = judgeReply;
        std::transform(upperReply.begin(), upperReply.end(), upperReply.begin(), ::toupper);
        return upperReply.find("DONG_THUAN") != std::string::npos
            || (upperReply.find("TRUE") != std::string::npos
                && upperReply.find("FALSE") == std::string::npos);
    }
}

// Hàm phụ trợ giúp xuất JSON để tái sử dụng cho nhiều Agent
void exportTrajectoryToJson(const Trajectory& traj, const std::string& suffix) {
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

    std::string filePath = outputDir + "/trajectory_" + traj.taskId + "_" + suffix + ".json";
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << std::setw(4) << trajJson << std::endl;
        outFile.close();
        std::cout << "[Main] Da luu Trajectory thanh cong tai: " << filePath << "\n";
    } else {
        std::cerr << "[LỖI] Khong the ghi file: " << filePath << "\n";
    }
}

int main() {
    std::cout << "==================================================\n";
    std::cout << " KHOI DONG HE THONG AI AGENT (CROSS-VERIFICATION) \n";
    std::cout << "==================================================\n\n";

    // 1. KHỞI TẠO LLM CLIENT CHO 2 AGENT
    string modelName1 = "google/gemma-4-26b-a4b-it:free";
    string modelName2 = "google/gemma-4-26b-a4b-it:free"; 
    
    OllamaClient llmClient1(modelName1, "https://openrouter.ai");
    OllamaClient llmClient2(modelName2, "https://openrouter.ai");

    // Message Queue để 2 luồng trả kết quả về luồng chính
    auto resultQueue = std::make_shared<ThreadSafeMessageQueue>();

    // Biến lưu trữ Trajectory của 2 Agent
    Trajectory traj1, traj2;

    std::string taskId = "test_cross_verification_001";
    std::string prompt = "Hãy chuyển đổi dòng text trong ngoặc này thành âm thanh 'Ronaldo là goat' BẮT BUỘC sử dụng công cụ text_to_speech để đọc to kết quả đó lên.";

    std::cout << "[Main] Giao viec song song cho 2 Agent: " << prompt << "\n\n";

    // ---------------------------------------------------------
    // 2. KÍCH HOẠT AGENT 1 (TRÊN LUỒNG 1)
    // ---------------------------------------------------------
    std::thread agent1Thread([&]() {
        NativeEnvironment env1;
        ToolRegistry registry1;
        
        registry1.registerTool(std::make_unique<TranslateTool>());
        registry1.registerTool(std::make_unique<WebSearchTool>());
        registry1.registerTool(std::make_unique<TextToSpeechTool>());
        
        // Khởi tạo SkillLoader và truyền vào AgentLoop thay vì nullptr
        SkillLoader skillLoader1("./skills");
        AgentLoop agent1(&llmClient1, &registry1, &skillLoader1, 10);
        
        // Cắm Hook cho Agent 1
        agent1.setStepHook([](const Step& step) {
            std::cout << "  \033[32m[Agent 1] Bước " << step.stepId 
                      << " | Tokens: " << step.tokensUsed 
                      << " | Latency: " << step.latencyMs << "ms\033[0m\n";
        });

        traj1 = agent1.run(taskId, prompt, "");
        resultQueue->push({"Agent_1", "MainAgent", traj1.finalAnswer});
    });

    // ---------------------------------------------------------
    // 3. KÍCH HOẠT AGENT 2 (TRÊN LUỒNG 2)
    // ---------------------------------------------------------
    std::thread agent2Thread([&]() {
        NativeEnvironment env2;
        ToolRegistry registry2;
        
        registry2.registerTool(std::make_unique<TranslateTool>());
        registry2.registerTool(std::make_unique<WebSearchTool>());
        registry2.registerTool(std::make_unique<TextToSpeechTool>());
        
        // Khởi tạo SkillLoader và truyền vào AgentLoop thay vì nullptr
        SkillLoader skillLoader2("./skills");
        AgentLoop agent2(&llmClient2, &registry2, &skillLoader2, 10);
        
        // Cắm Hook cho Agent 2
        agent2.setStepHook([](const Step& step) {
            std::cout << "  \033[33m[Agent 2] Bước " << step.stepId 
                      << " | Tokens: " << step.tokensUsed 
                      << " | Latency: " << step.latencyMs << "ms\033[0m\n";
        });

        traj2 = agent2.run(taskId, prompt, "");
        resultQueue->push({"Agent_2", "MainAgent", traj2.finalAnswer});
    });

    // ---------------------------------------------------------
    // 4. LẤY KẾT QUẢ VÀ ĐỢI TIẾN TRÌNH HOÀN TẤT
    // ---------------------------------------------------------
    AgentMessage resA = resultQueue->pop();
    AgentMessage resB = resultQueue->pop();

    if (agent1Thread.joinable()) agent1Thread.join();
    if (agent2Thread.joinable()) agent2Thread.join();

    // 5. HIỂN THỊ KẾT QUẢ CƠ BẢN VÀ SO SÁNH CHÉO
    std::cout << "\n==================================================\n";
    std::cout << "[KẾT QUẢ AGENT TRẢ VỀ & SO SÁNH CHÉO]\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << "[Agent 1 (" << traj1.model << ")]\n";
    std::cout << "- Success: " << (traj1.success ? "True" : "False") << "\n";
    std::cout << "- Answer: " << traj1.finalAnswer << "\n";
    std::cout << "- Time: " << traj1.totalTimeMs << " ms\n\n";

    std::cout << "[Agent 2 (" << traj2.model << ")]\n";
    std::cout << "- Success: " << (traj2.success ? "True" : "False") << "\n";
    std::cout << "- Answer: " << traj2.finalAnswer << "\n";
    std::cout << "- Time: " << traj2.totalTimeMs << " ms\n";
    std::cout << "--------------------------------------------------\n";
    
    // ---------------------------------------------------------
    // ĐÁNH GIÁ ĐỒNG THUẬN BẰNG LLM (LLM-as-a-Judge)
    // ---------------------------------------------------------
    if (traj1.success && traj2.success) {
        std::cout << "[Main] Dang gui ket qua cho LLM Judge danh gia...\n";
        
        // Sử dụng client2 làm trọng tài chấm điểm
        // Truyền thêm `prompt` (nhiệm vụ gốc) để Judge biết ngữ cảnh, tránh báo
        // XUNG DOT oan khi 2 agent cùng thành công nhưng khác chi tiết phụ (tên file...).
        bool is_consensus = evaluateConsensus(llmClient2, prompt, traj1.finalAnswer, traj2.finalAnswer);

        if (is_consensus) {
            std::cout << "=> ĐÁNH GIÁ (LLM Judge): DONG THUAN. Hai Agent cung cung cap mot du kien cot loi.\n";
        } else {
            std::cout << "=> ĐÁNH GIÁ (LLM Judge): XUNG DOT. Ket qua can kiem duyet lai.\n";
        }
    } else {
        std::cout << "=> ĐÁNH GIÁ: Khong the danh gia dong thuan vi co it nhat 1 Agent that bai.\n";
    }
    std::cout << "==================================================\n";

    // 6. XUẤT FILE TRAJECTORY.JSON RA THƯ MỤC
    exportTrajectoryToJson(traj1, "nvidia");
    exportTrajectoryToJson(traj2, "gemma");

    return 0;
}