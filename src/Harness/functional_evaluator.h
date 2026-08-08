#pragma once
#include "evaluator.h"
#include <iostream>
#include <string>
#include <format>


class FunctionalEvaluator : public Evaluator {
private:
    std::string evalScript_;
    int rewardScore_ = 100;
    bool isPassed_ = false;
    std::string lastResult_;
public:
    FunctionalEvaluator(const std::string& script, int rewardScore) : evalScript_(script), rewardScore_(rewardScore)  {}

    std::string getName() const override { return "FunctionalEvaluator"; }

    bool isPassed() const override { return isPassed_; }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& traj, Environment* env) override {
        (void)finalAnswer;
        (void)traj;
        if (!env) return std::nullopt;
        if (evalScript_.empty()) return std::nullopt;

        // Chạy script kiểm tra hệ thống do file JSON cung cấp
        std::string result = env->executeCommand(evalScript_);
        lastResult_ =result;
        // Cần có logic để phân tích result (ví dụ: trả về 1 là pass)
        // Ở đây giả lập đơn giản: Nếu kết quả có chữ PASS -> trả về rewardScore
        if (result.find("PASS") != std::string::npos) {
            isPassed_ = true;
            return rewardScore_;
        }

        isPassed_ = false;
        return 0;
    }
    std::string getDetail() const override {
        return format("Script output: {}\n", lastResult_);
    }
};
