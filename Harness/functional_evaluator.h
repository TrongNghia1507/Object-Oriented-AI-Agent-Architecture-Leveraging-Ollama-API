#pragma once
#include "evaluator.h"
#include <iostream>
#include <string>


class FunctionalEvaluator : public Evaluator {
private:
    std::string evalScript_;
    int rewardScore_ ;
    bool isPassed_ = false;
public:
    FunctionalEvaluator(const std::string& script) : evalScript_(script) {}

    std::string getName() const override { return "FunctionalEvaluator"; }

    bool isPassed() const override { return isPassed_; }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& traj, Environment* env) override {
        (void)finalAnswer;
        (void)traj;
        if (!env) return std::nullopt;
        if (evalScript_.empty()) return std::nullopt;

        // Chạy script kiểm tra hệ thống do file JSON cung cấp
        std::string result = env->executeCommand(evalScript_);
        
        // Cần có logic để phân tích result (ví dụ: trả về 1 là pass)
        // Ở đây giả lập đơn giản: Nếu kết quả có chữ PASS -> trả về rewardScore
        if (result.find("PASS") != std::string::npos) {
            isPassed_ = true;
            return rewardScore_;
        }

        isPassed_ = false;
        return 0;
    }
};
