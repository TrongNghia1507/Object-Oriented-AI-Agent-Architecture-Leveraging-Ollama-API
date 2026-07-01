#pragma once
#include "evaluator.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

// ─── KeywordEvaluator — Bộ chấm điểm dựa trên từ khóa ────────────────────────
//
// Lớp này kế thừa từ Evaluator (Strategy Pattern).
// Nhiệm vụ: Kiểm tra xem câu trả lời cuối cùng (finalAnswer) của Agent
// có chứa các từ khóa được yêu cầu hay không. Mỗi từ khóa tìm thấy sẽ được cộng điểm.

class KeywordEvaluator : public Evaluator {
private:
    std::vector<std::string> expectedKeywords_;
    int scorePerKeyword_;

    // Hàm phụ trợ: Chuyển chuỗi sang chữ in thường (Lowercase)
    // Giúp việc chấm điểm chính xác hơn, không bị trượt do lỗi viết Hoa/thường
    std::string toLowerCase(std::string s) const {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }

public:
    /**
     * @brief Khởi tạo bộ đánh giá từ khóa.
     * @param keywords Danh sách các từ khóa kỳ vọng.
     * @param score Điểm số cộng thêm cho mỗi từ khóa tìm thấy (mặc định là 1).
     */
    KeywordEvaluator(const std::vector<std::string>& keywords, int score = 1)
        : expectedKeywords_(keywords), scorePerKeyword_(score) {}

    // Ghi đè hàm getName() từ lớp cha
    std::string getName() const override {
        return "KeywordEvaluator";
    }

    // === SỬA ĐỔI CHÍNH: Thêm tham số "Environment* env" vào cuối ===
    // Ghi đè hàm evaluate() với kiểu trả về std::optional<int> theo chuẩn C++17
    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& trajectory, Environment* env) override {
        // Mẹo C++: Báo cho trình biên dịch biết biến env cố tình không được dùng
        // để tránh cảnh báo (warning: unused parameter)
        (void)env; 

        // Nếu câu trả lời rỗng, có thể coi như không thể chấm điểm hoặc điểm 0
        if (finalAnswer.empty()) {
            return 0; // C++17 sẽ tự động gói số 0 này vào std::optional
        }

        int totalScore = 0;
        
        // Chuẩn hóa câu trả lời của Agent về chữ thường
        std::string normalizedAnswer = toLowerCase(finalAnswer);

        // Quét từng từ khóa
        for (const auto& kw : expectedKeywords_) {
            std::string normalizedKw = toLowerCase(kw);
            
            // Nếu tìm thấy từ khóa trong câu trả lời -> Cộng điểm
            if (normalizedAnswer.find(normalizedKw) != std::string::npos) {
                totalScore += scorePerKeyword_;
            }
        }
        
        // Trả về tổng điểm. Trình biên dịch C++17 sẽ tự động convert kiểu 'int' thành 'std::optional<int>'
        return totalScore; 
    }
};