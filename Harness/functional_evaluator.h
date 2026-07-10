#pragma once
#include "evaluator.h"
#include <iostream>
#include <string>


class FunctionalEvaluator : public Evaluator {
private:
    std::string evalScript_;
    bool isPassed_ = false;
public:
    FunctionalEvaluator(const std::string& script) : evalScript_(script) {}

    std::string getName() const override { return "FunctionalEvaluator"; }

    bool isPassed() const override { return isPassed_; }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& traj, Environment* env) override {
        (void)finalAnswer;
        (void)traj;
        if (!env) return std::nullopt;

        // Chạy script kiểm tra hệ thống do file JSON cung cấp
        std::string result = env->executeCommand(evalScript_);
        
        // Cần có logic để phân tích result (ví dụ: trả về 1 là pass)
        // Ở đây giả lập đơn giản: Nếu kết quả có độ dài > 0 và không có chữ "Loi"
        if (!result.empty() && result.find("Loi") == std::string::npos && result.find("Error") == std::string::npos) {
            isPassed_ = true;
            return 100; // Đạt điểm tối đa
        }
        
        isPassed_ = false;
        return 0;
    }
};

// === CẬP NHẬT: Lớp FunctionalMemoryEvaluator cũ ===
class FunctionalMemoryEvaluator : public Evaluator {
private:
    int rewardScore_;
    bool isPassed_ = false;
public:
    FunctionalMemoryEvaluator(int score = 50) : rewardScore_(score) {}

    std::string getName() const override { return "FunctionalMemoryEvaluator"; }
    
    bool isPassed() const override { return isPassed_; } // Thêm hàm bắt buộc

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& traj, Environment* env) override {
        (void)finalAnswer;
        (void)traj;

        if (!env) {
            std::cerr << "  [-] [Functional Eval] Loi: Khong co Environment!\n";
            return std::nullopt;
        }

        std::cout << "  -> [Functional Eval] Dang vao hien truong de check DB...\n";
        
        std::string sqlCmd = "sqlite3 agent_memory.db \"SELECT COUNT(*) FROM agent_history WHERE content LIKE '%Ha Noi%';\"";
        
        // SỬA LỖI: Đổi executeEvalCommand thành executeCommand
        std::string result = env->executeCommand(sqlCmd);

        std::cout << "  -> [Functional Eval] Output OS tra ve: '" << result << "'\n";

        try {
            int count = std::stoi(result);
            if (count > 0) {
                std::cout << "  -> [Functional Eval] Phat hien " << count << " dong du lieu hop le! Cong diem.\n";
                isPassed_ = true;
                return rewardScore_; 
            }
        } catch (...) {
            std::cout << "  -> [Functional Eval] Khong the doc duoc du lieu hoac DB trong rong.\n";
        }

        isPassed_ = false;
        return 0; 
    }
};