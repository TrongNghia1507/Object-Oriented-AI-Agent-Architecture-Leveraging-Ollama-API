#pragma once
#include "evaluator.h"
#include <string>
#include <optional>
#include "../LLMClient/json.hpp" // Thêm thư viện JSON để dùng Factory

// === 1. FunctionalEvaluator: Chấm điểm bằng Bash/Shell Script ===
class FunctionalEvaluator : public Evaluator {
private:
    std::string evalScript_;
    std::string expectedOutput_; // Cho phép tùy biến chữ "PASS" hoặc chuỗi khác
    int rewardScore_;
    bool isPassed_ = false;
public:
    FunctionalEvaluator(const std::string& script, const std::string& expectedOutput = "PASS", int score = 50) 
        : evalScript_(script), expectedOutput_(expectedOutput), rewardScore_(score) {}

    std::string getName() const override { return "FunctionalEvaluator"; }
    bool isPassed() const override { return isPassed_; }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& traj, Environment* env) override {
        (void)finalAnswer;
        (void)traj;
        if (!env || evalScript_.empty()) return std::nullopt;

        // Chạy script kiểm tra một cách thầm lặng
        std::string result = env->executeCommand(evalScript_);
        
        // So sánh với expectedOutput linh hoạt
        if (result.find(expectedOutput_) != std::string::npos) {
            isPassed_ = true;
            return rewardScore_;
        }

        isPassed_ = false;
        return 0;
    }
};

// === 2. FunctionalSqlEvaluator (Thay thế FunctionalMemoryEvaluator cũ) ===
// Cho phép truyền vào bất kỳ câu lệnh SQL nào và yêu cầu trả về số lượng row > 0
class FunctionalSqlEvaluator : public Evaluator {
private:
    std::string sqlQuery_;
    int rewardScore_;
    bool isPassed_ = false;
    std::string dbPath_; 
public:
    FunctionalSqlEvaluator(const std::string& sqlQuery, int score = 50, const std::string& dbPath = "memory.db") 
        : sqlQuery_(sqlQuery), rewardScore_(score), dbPath_(dbPath) {}

    std::string getName() const override { return "FunctionalSqlEvaluator"; }
    bool isPassed() const override { return isPassed_; }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& traj, Environment* env) override {
        (void)finalAnswer;
        (void)traj;

        if (!env || sqlQuery_.empty()) return std::nullopt;

        // Lệnh chạy SQLite thầm lặng không in log
        std::string sqlCmd = "sqlite3 " + dbPath_ + " \"" + sqlQuery_ + "\"";
        std::string result = env->executeCommand(sqlCmd);

        try {
            int count = std::stoi(result);
            if (count > 0) {
                isPassed_ = true;
                return rewardScore_; 
            }
        } catch (...) {
            // Nuốt lỗi thầm lặng nếu DB trống hoặc lỗi parse
        }

        isPassed_ = false;
        return 0; 
    }
};