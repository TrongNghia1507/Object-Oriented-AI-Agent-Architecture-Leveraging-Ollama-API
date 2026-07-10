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
        
        // === BỔ SUNG QUAN TRỌNG: Ghi lại Action và Observation ===
        // Nếu không có 2 dòng này, file JSON của bạn sẽ không biết AI đã xài Tool gì!
        stepNode["action"] = step.actionLog;
        stepNode["observation"] = step.observation;
        
        // (Tùy chọn) Ghi thêm thời gian trễ để báo cáo xịn hơn
        if (step.latencyMs > 0) {
            stepNode["latency_ms"] = step.latencyMs;
        }
        // =========================================================

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
void HarnessRunner::loadTaskFromJson(const std::string& filename, AgentLoop& agent) {
    // 1. Đọc file JSON chứa danh sách task (định dạng mục 7.2 đề bài)
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cerr << "[HARNESS RUNNER] Loi: khong mo duoc file task '" << filename << "'\n";
        return;
    }

    nlohmann::json taskList;
    try {
        inFile >> taskList;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[HARNESS RUNNER] Loi parse JSON: " << e.what() << "\n";
        return;
    }
    if (!taskList.is_array()) {
        std::cerr << "[HARNESS RUNNER] Loi: file task phai la JSON array.\n";
        return;
    }

    int totalTasks = 0, passedTasks = 0;
    nlohmann::json batchSummary = nlohmann::json::array();

    // 2. Duyệt và chạy từng task trong benchmark
    for (const auto& taskDef : taskList) {
        std::string taskId      = taskDef.value("id", "unknown_task");
        std::string instruction = taskDef.value("instruction",
                                     taskDef.value("description", ""));
        std::string evalType    = taskDef.value("eval_type", "keyword");
        int maxSteps            = taskDef.value("max_steps", 10);

        std::cout << "\n[HARNESS RUNNER] --> Task '" << taskId
                   << "' (eval_type=" << evalType
                   << ", max_steps=" << maxSteps << ")\n";

        agent.setMaxSteps(maxSteps);

        // Registry/Factory pattern: build evaluator đúng theo eval_type của từng task
        clearEvaluators();
        if (evalType == "keyword") {
            addEvaluator(KeywordEvaluator::fromTaskJson(taskDef));
        } else if (evalType == "functional") {
            // [CẦN THÊM] class FunctionalEvaluator tổng quát nhận eval_script,
            // xem gợi ý bên dưới — thay cho FunctionalMemoryEvaluator hard-code hiện tại
            std::string evalScript = taskDef.value("eval_script", "");
            addEvaluator(std::make_shared<FunctionalEvaluator>(evalScript));
        } else {
            std::cerr << "  [!] eval_type '" << evalType << "' khong ho tro, bo qua.\n";
            continue;
        }

        runTask(taskId, instruction, agent);   // tái dùng pipeline setup->run->evaluate->record

        totalTasks++;
        // [CẦN THÊM] isPassed() phải được khai báo ảo (virtual) trong Evaluator gốc
        bool passed = !evaluators_.empty() && evaluators_.front()->isPassed();
        if (passed) passedTasks++;

        batchSummary.push_back({
            {"task_id", taskId}, {"eval_type", evalType}, {"passed", passed}
        });
    }

    // 3. Xuất báo cáo success rate tổng hợp (yêu cầu 3.6 & 7.3)
    double successRate = totalTasks ? (100.0 * passedTasks / totalTasks) : 0.0;
    nlohmann::json finalReport = {
        {"total_tasks", totalTasks},
        {"passed_tasks", passedTasks},
        {"success_rate_percent", successRate},
        {"tasks", batchSummary}
    };
    std::ofstream summaryFile("benchmark_summary.json");
    if (summaryFile.is_open()) {
        summaryFile << std::setw(4) << finalReport << std::endl;
    }

    std::cout << "\n[HARNESS RUNNER] ===== BATCH EVALUATION =====\n"
               << "  Tong task: " << totalTasks << " | PASS: " << passedTasks
               << " | Success rate: " << successRate << "%\n"
               << "  Da xuat: benchmark_summary.json\n";
}