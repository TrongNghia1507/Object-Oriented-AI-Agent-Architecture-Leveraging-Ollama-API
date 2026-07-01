#include "Harness_runner.h"
#include <iostream>
#include <fstream>
#include <iomanip>

void HarnessRunner::addEvaluator(std::shared_ptr<Evaluator> evaluator) {
    if (evaluator) {
        evaluators_.push_back(evaluator);
    }
}

void HarnessRunner::runTask(const std::string& taskId, const std::string& task, AgentLoop& agent) {
    std::cout << "\n==================================================\n";
    std::cout << "[HARNESS RUNNER] Bat dau kiem thu Task ID: " << taskId << "\n";
    std::cout << "==================================================\n";

    // === 1. THIẾT LẬP MÔI TRƯỜNG TRƯỚC TIÊN (Bổ sung mới) ===
    if (env_) {
        env_->setup(); 
    }

    // 2. Khởi tạo cấu trúc cây JSON để xuất báo cáo quỹ đạo
    nlohmann::json reportJson = nlohmann::json::object();
    reportJson["task_id"] = taskId;
    reportJson["prompt"] = task;
    reportJson["steps"] = nlohmann::json::array();

    int stepCounter = 0;

    // 3. TIÊM HOOK GIÁM SÁT BÍ MẬT
    agent.setStepHook([&reportJson, &stepCounter](const Step& step) {
        stepCounter++;
        nlohmann::json stepNode = nlohmann::json::object();
        stepNode["step_idx"] = stepCounter;
        stepNode["thought"] = step.thought;
        reportJson["steps"].push_back(stepNode);
    });

    // 4. KÍCH HOẠT AGENT LOOP CHẠY TỰ NHIÊN
    Trajectory trajectory = agent.run(taskId, task);

    // GỠ HOOK NGAY SAU KHI HOÀN THÀNH
    agent.setStepHook(nullptr);

    std::string finalAnswer = trajectory.finalAnswer; 

    // === 5. TIẾN HÀNH CHẤM ĐIỂM TỰ ĐỘNG (SỬA DÒNG LỖI TẠI ĐÂY) ===
    std::cout << "\n[HARNESS RUNNER] Dang kich hoat cac bo cham diem...\n";
    int totalScore = 0;
    nlohmann::json evalBreakdown = nlohmann::json::object();

    for (const auto& evaluator : evaluators_) {
        // CHÍNH XÁC: Truyền thêm tham số thứ 3 là env_.get() để chuyển giao con trỏ Environment thô
        std::optional<int> scoreResult = evaluator->evaluate(finalAnswer, trajectory, env_.get());
        
        if (scoreResult.has_value()) {
            int score = scoreResult.value();
            totalScore += score;
            evalBreakdown[evaluator->getName()] = score;
            std::cout << "  [+] " << evaluator->getName() << ": " << score << " diem.\n";
        } else {
            evalBreakdown[evaluator->getName()] = "Nullopt";
            std::cout << "  [-] " << evaluator->getName() << ": Khong the cham diem (nullopt).\n";
        }
    }

    std::cout << "[HARNESS RUNNER] ---> Tong diem hieu nang dat duoc: " << totalScore << " diem.\n";

    // ĐÓNG GÓI DỮ LIỆU JSON
    reportJson["final_answer"] = finalAnswer;
    reportJson["evaluation_results"] = {
        {"total_score", totalScore},
        {"breakdown", evalBreakdown}
    };

    std::string outputFilename = "trajectory_" + taskId + ".json";
    std::ofstream outFile(outputFilename);
    if (outFile.is_open()) {
        outFile << std::setw(4) << reportJson << std::endl;
        outFile.close();
    }

    // === 6. DỌN DẸP MÔI TRƯỜNG SAU KHI THI XONG (Bổ sung mới) ===
    if (env_) {
        env_->teardown(); 
    }
    
    std::cout << "==================================================\n\n";
}