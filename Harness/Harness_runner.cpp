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
    std::cout << "[HARNESS RUNNER] Yeu cau (Prompt): " << task << "\n";
    std::cout << "==================================================\n";

    // 1. Khởi tạo cấu trúc cây JSON để xuất báo cáo quỹ đạo (Trajectory Output)
    nlohmann::json reportJson = nlohmann::json::object();
    reportJson["task_id"] = taskId;
    reportJson["prompt"] = task;
    reportJson["steps"] = nlohmann::json::array();

    int stepCounter = 0;

    // 2. TIÊM HOOK GIÁM SÁT BÍ MẬT (Observer Pattern / Hook)
    // Cứ mỗi khi AgentLoop chạy xong một bước, hàm lambda này tự động hứng dữ liệu.
    agent.setStepHook([&reportJson, &stepCounter](const Step& step) {
        stepCounter++;
        std::cout << "[Harness Monitor] Ghi nhan ngam Buoc thu " << stepCounter << " thanh cong.\n";
        
        // Tạo một nút JSON ghi nhật ký cho bước hiện tại
        nlohmann::json stepNode = nlohmann::json::object();
        stepNode["step_idx"] = stepCounter;
        
        // LƯU Ý GỘP CODE: Nếu file 'step.h' của bạn có các trường public như thought, action, observation...
        // Bạn hãy bỏ comment các dòng dưới đây để lưu chi tiết tuyệt đối vào file JSON:
        // stepNode["thought"] = step.thought;
        // stepNode["action"] = step.action;
        
        reportJson["steps"].push_back(stepNode);
    });

    // 3. KÍCH HOẠT AGENT LOOP CHẠY TỰ NHIÊN (Luồng ReAct độc lập hoàn toàn)
    Trajectory trajectory = agent.run(taskId, task);

    // 4. GỠ HOOK NGAY SAU KHI HOÀN THÀNH (Đảm bảo an toàn bộ nhớ tuyệt đối)
    agent.setStepHook(nullptr);

    // LƯU Ý GỘP CODE: Tùy thuộc vào việc struct Trajectory của nhóm thiết kế lưu câu trả lời cuối cùng ở đâu,
    // hãy gán chuỗi đó vào biến finalAnswer dưới đây (Ví dụ: trajectory.finalAnswer hoặc trajectory.getFinalAnswer())
    std::string finalAnswer = "Ket qua mo phong cau tra loi cuoi cung tu Agent."; 

    std::cout << "\n[HARNESS RUNNER] Vong lap Agent ket thuc.\n";
    std::cout << "[HARNESS RUNNER] Cau tra loi cuoi cung cua Agent:\n> " << finalAnswer << "\n";

    // 5. TIẾN HÀNH CHẤM ĐIỂM TỰ ĐỘNG (Duyệt qua các tiêu chí Đa hình Strategy)
    std::cout << "\n[HARNESS RUNNER] Dang kich hoat cac bo cham diem...\n";
    int totalScore = 0;
    nlohmann::json evalBreakdown = nlohmann::json::object();

    for (const auto& evaluator : evaluators_) {
        // Thực thi hàm evaluate nhận về std::optional<int> chuẩn C++17
        std::optional<int> scoreResult = evaluator->evaluate(finalAnswer, trajectory);
        
        // Kiểm tra xem chiếc hộp optional có chứa giá trị hay không (An toàn dữ liệu)
        if (scoreResult.has_value()) {
            int score = scoreResult.value();
            totalScore += score;
            evalBreakdown[evaluator->getName()] = score;
            std::cout << "  [+] " << evaluator->getName() << ": " << score << " diem.\n";
        } else {
            evalBreakdown[evaluator->getName()] = "Nullopt (Loi bieu do/Khong the danh gia)";
            std::cout << "  [-] " << evaluator->getName() << ": Khong the cham diem (nullopt).\n";
        }
    }

    std::cout << "[HARNESS RUNNER] ---> Tong diem hieu nang dat duoc: " << totalScore << " diem.\n";

    // 6. ĐÓNG GÓI TOÀN BỘ DỮ LIỆU VÀ XUẤT FILE JSON BÁO CÁO
    reportJson["final_answer"] = finalAnswer;
    reportJson["evaluation_results"] = {
        {"total_score", totalScore},
        {"breakdown", evalBreakdown}
    };

    std::string outputFilename = "trajectory_" + taskId + ".json";
    std::ofstream outFile(outputFilename);
    if (outFile.is_open()) {
        // Xuất file thụt lề định dạng 4 khoảng trắng cực kỳ chuẩn chỉnh và đẹp mắt
        outFile << std::setw(4) << reportJson << std::endl;
        outFile.close();
        std::cout << "[HARNESS RUNNER] Da ghi nhan va ket xuat file bao cao: " << outputFilename << "\n";
    } else {
        std::cerr << "[HARNESS RUNNER] LOI NGHIEM TRONG: Khong the tao file tap lenh " << outputFilename << "\n";
    }
    std::cout << "==================================================\n\n";
}