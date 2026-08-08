#pragma once
#include "evaluator.h"
#include "../LLMClient/json.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <format>
// ─── KeywordEvaluator — Bộ chấm điểm dựa trên từ khóa ────────────────────────
//
// Strategy Pattern: cùng interface Evaluator::evaluate() với FunctionalEvaluator.
// Nhiệm vụ: đếm số từ khóa kỳ vọng xuất hiện trong finalAnswer, cộng điểm
// theo từng từ, và quyết định PASS/FAIL dựa trên ngưỡng minRequired_.

class KeywordEvaluator : public Evaluator {
private:
    std::vector<std::string> expectedKeywords_;
    int scorePerKeyword_;
    int minRequired_;      // == THÊM MỚI: số từ khóa tối thiểu để coi là "đạt"
    int matchedCount_ = 0; // == THÊM MỚI: lưu lại số từ khóa khớp được của lần evaluate gần nhất

    std::string toLowerCase(std::string s) const {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }

public:
    /**
     * @param keywords    Danh sách các từ khóa kỳ vọng.
     * @param score       Điểm cộng thêm cho mỗi từ khóa tìm thấy.
     * @param minRequired Số từ khóa tối thiểu cần khớp để coi là PASS.
     *                    Mặc định = toàn bộ keywords (khớp hết mới đạt).
     */
    KeywordEvaluator(const std::vector<std::string>& keywords, int score = 1, int minRequired = -1)
        : expectedKeywords_(keywords),
          scorePerKeyword_(score),
          minRequired_(minRequired < 0 ? static_cast<int>(keywords.size()) : minRequired) {}

    // == THÊM MỚI: Factory method — build trực tiếp từ 1 phần tử tasks.json ==
    // Cho phép HarnessRunner build evaluator đúng theo từng task, thay vì
    // dùng chung 1 danh sách từ khóa cứng cho cả batch.
    // Kỳ vọng task JSON có dạng:
    //   { "eval_type": "keyword",
    //     "expected_keywords": ["Hà Nội", "nắng"],
    //     "score_per_keyword": 10,
    //     "min_required": 2 }
    static std::shared_ptr<KeywordEvaluator> fromTaskJson(const nlohmann::json& taskDef) {
        std::vector<std::string> keywords;
        if (taskDef.contains("expected_keywords") && taskDef["expected_keywords"].is_array()) {
            for (const auto& kw : taskDef["expected_keywords"]) {
                keywords.push_back(kw.get<std::string>());
            }
        }
        int score = taskDef.value("score_per_keyword", 1);
        int minRequired = taskDef.value("min_required", -1);
        return std::make_shared<KeywordEvaluator>(keywords, score, minRequired);
    }

    std::string getName() const override {
        return "KeywordEvaluator";
    }

    // == THÊM MỚI: cho HarnessRunner biết task này có PASS hay không ==
    bool isPassed() const override {
        return matchedCount_ >= minRequired_;
    }

    int matchedCount() const { return matchedCount_; }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& trajectory, Environment* env) override {
        (void)env;
        (void)trajectory;

        matchedCount_ = 0;

        if (expectedKeywords_.empty()) {
            return std::nullopt; // không có gì để chấm -> không chấm được
        }

        if (finalAnswer.empty()) {
            return 0;
        }

        std::string normalizedAnswer = toLowerCase(finalAnswer);
        int totalScore = 0;

        for (const auto& kw : expectedKeywords_) {
            std::string normalizedKw = toLowerCase(kw);
            if (normalizedAnswer.find(normalizedKw) != std::string::npos) {
                totalScore += scorePerKeyword_;
                ++matchedCount_;
            }
        }

        return totalScore;
    }
    std::string getDetail() const override {
    return format("Khop {}/{} tu khoa (can toi thieu {})",std::to_string(matchedCount_),std::to_string(expectedKeywords_.size()),std::to_string(minRequired_));
}
};