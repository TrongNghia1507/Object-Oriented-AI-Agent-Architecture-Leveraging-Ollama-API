#include "Harness_runner.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem> // Bổ sung thư viện filesystem để làm Sandbox

namespace fs = std::filesystem;

void HarnessRunner::addEvaluator(std::shared_ptr<Evaluator> evaluator) {
    if (evaluator) {
        evaluators_.push_back(evaluator);
    }
}

bool HarnessRunner::runTask(const std::string& taskId, const std::string& task, AgentLoop& agent, const nlohmann::json& taskDef) {
    std::cout << "\n==================================================\n";
    std::cout << "[HARNESS RUNNER] Bat dau kiem thu Task ID: " << taskId << "\n";
    std::cout << "==================================================\n";

    // === 0. TẠO VÀ CHUYỂN VÀO SANDBOX (CÁCH LY AGENT) ===
    // Lưu lại thư mục gốc để quay về sau khi test xong
    std::string originalDir = fs::current_path().string();
    std::string sandboxDir = "/tmp/sandbox_" + taskId;
    
    // Xóa sandbox cũ (nếu có) để đảm bảo môi trường sạch 100%
    if (fs::exists(sandboxDir)) {
        fs::remove_all(sandboxDir); 
    }
    fs::create_directories(sandboxDir);
    
    // Đẩy Agent vào sandbox. Khi Agent chạy lệnh 'ls -la', nó chỉ thấy thư mục trống.
    fs::current_path(sandboxDir);

    // === 0.5 TỰ ĐỘNG KHỞI TẠO FILE TỪ CẤU HÌNH JSON (DYNAMIC SETUP) ===
    if (taskDef.contains("setup_files") && taskDef["setup_files"].is_array()) {
        for (const auto& fileDef : taskDef["setup_files"]) {
            std::string destPath = fileDef.value("path", ""); // Tên file trong Sandbox (VD: "data.txt")
            
            if (destPath.empty()) continue;

            // CÁCH 1: NẾU CÓ ĐƯỜNG DẪN FILE NGUỒN -> COPY FILE THẬT VÀO SANDBOX
            if (fileDef.contains("src")) {
                std::string srcPath = fileDef.value("src", "");
                if (fs::exists(srcPath)) {
                    fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing);
                    std::cout << "  [Harness Setup] Da copy file tu '" << srcPath << "' -> '" << destPath << "'\n";
                } else {
                    std::cerr << "  [Harness Setup] Loi: Khong tim thay file nguon: " << srcPath << "\n";
                }
            } 
            // CÁCH 2: NẾU CÓ KHAI BÁO CONTENT -> TỰ TẠO FILE TẠM
            else if (fileDef.contains("content")) {
                std::string content = fileDef.value("content", "");
                std::ofstream outFile(destPath);
                if (outFile.is_open()) {
                    outFile << content;
                    outFile.close();
                    std::cout << "  [Harness Setup] Da tao file tu chuoi content: " << destPath << "\n";
                }
            }
        }
    }

    // === 1. THIẾT LẬP MÔI TRƯỜNG TRƯỚC TIÊN ===
    if (env_) {
        env_->setup();
    }

    // 2. TIÊM HOOK GIÁM SÁT 
    agent.setStepHook([](const Step& step) {
        std::cout << " step_idx" << step.stepId
                   << " ghi nhan (" << step.latencyMs << "ms)\n";
    });

    // 3. KÍCH HOẠT AGENT LOOP CHẠY TỰ NHIÊN
    // Agent sẽ hoàn toàn không biết nó đang ở trong /tmp/sandbox_...
    Trajectory trajectory = agent.run(taskId, task);

    // GỠ HOOK NGAY SAU KHI HOÀN THÀNH
    agent.setStepHook(nullptr);

    std::string finalAnswer = trajectory.finalAnswer;

    // === 4. TIẾN HÀNH CHẤM ĐIỂM TỰ ĐỘNG ===
    std::cout << "\n[HARNESS RUNNER] Dang kich hoat cac bo cham diem...\n";
    int totalScore = 0;
    nlohmann::json evalBreakdown = nlohmann::json::object();

    for (const auto& evaluator : evaluators_) {
        std::optional<int> scoreResult = evaluator->evaluate(finalAnswer, trajectory, env_);

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

    // === 5. DỌN DẸP MÔI TRƯỜNG SAU KHI THI XONG ===
    if (env_) {
        env_->teardown();
    }

    // === 6. THOÁT KHỎI SANDBOX VÀ XUẤT JSON ===
    // Bắt buộc phải quay lại thư mục gốc để lưu file JSON đúng chỗ
    fs::current_path(originalDir);

    nlohmann::json reportJson = trajectory.toJson();
    reportJson["evaluation_results"] = {
        {"total_score", totalScore},
        {"breakdown", evalBreakdown}
    };

    // Đẩy file log ra thư mục an toàn, tránh để lại trong sandbox
    std::string outputFilename = "trajectory_" + taskId + ".json";
    std::ofstream outFile(outputFilename);
    if (outFile.is_open()) {
        outFile << std::setw(4) << reportJson << std::endl;
        outFile.close();
    }

    std::cout << "==================================================\n\n";

    bool anyEvaluatorPassed = false;
    bool anyEvaluatorRan = false;
    for (const auto& evaluator : evaluators_) {
        anyEvaluatorRan = true;
        if (evaluator->isPassed()) {
            anyEvaluatorPassed = true;
            break;
        }
    }
    return anyEvaluatorRan ? anyEvaluatorPassed : trajectory.success;
}

void HarnessRunner::loadTaskFromJson(const std::string& filename, AgentLoop& agent) {
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

        // Registry/Factory pattern được cập nhật để bắt tham số linh hoạt
        clearEvaluators();
        if (evalType == "keyword") {
            addEvaluator(KeywordEvaluator::fromTaskJson(taskDef));
        } else if (evalType == "functional") {
            std::string evalScript = taskDef.value("eval_script", "");
            std::string expectedOut = taskDef.value("expected_output", "PASS");
            int score = taskDef.value("score", 50);
            addEvaluator(std::make_shared<FunctionalEvaluator>(evalScript, expectedOut, score));
        } else if (evalType == "sql") {
            // Tích hợp Evaluator thao tác DB linh hoạt thay cho bản hardcode
            std::string sqlQuery = taskDef.value("sql_query", "");
            int score = taskDef.value("score", 50);
            std::string dbPath = taskDef.value("db_path", "memory.db");
            addEvaluator(std::make_shared<FunctionalSqlEvaluator>(sqlQuery, score, dbPath));
        } else {
            std::cerr << "  [!] eval_type '" << evalType << "' khong ho tro, bo qua.\n";
            continue;
        }

        // Truyền thêm taskDef vào để hệ thống biết có cần copy file mẫu vào sandbox không
        bool passed = runTask(taskId, instruction, agent, taskDef);

        totalTasks++;
        if (passed) passedTasks++;

        batchSummary.push_back({
            {"task_id", taskId}, {"eval_type", evalType}, {"passed", passed}
        });
    }

    double successRate = totalTasks ? (100.0 * passedTasks / totalTasks) : 0.0;
    nlohmann::json finalReport = {
        {"total_tasks", totalTasks},
        {"passed_tasks", passedTasks},
        {"success_rate_percent", successRate},
        {"tasks", batchSummary}
    };
    
    // Lưu báo cáo an toàn ở thư mục gốc (không nằm trong sandbox)
    std::ofstream summaryFile("/workspaces/AgentAI/benchmark_summary/benchmark_summary.json");
    if (summaryFile.is_open()) {
        summaryFile << std::setw(4) << finalReport << std::endl;
    }

    std::cout << "\n[HARNESS RUNNER] ===== BATCH EVALUATION =====\n"
               << "  Tong task: " << totalTasks << " | PASS: " << passedTasks
               << " | Success rate: " << successRate << "%\n"
               << "  Da xuat: benchmark_summary.json\n";
}