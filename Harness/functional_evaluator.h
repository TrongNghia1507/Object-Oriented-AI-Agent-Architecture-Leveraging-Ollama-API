#pragma once
#include "evaluator.h"
#include <iostream>
#include <string>

// ─── FunctionalMemoryEvaluator — Bộ chấm điểm dựa trên Trạng thái Môi trường ──
//
// Lớp này kế thừa từ Evaluator (Strategy Pattern).
// Nhiệm vụ: Bỏ qua lời nói của AI, trực tiếp dùng Environment để truy vấn
// vào tệp agent_memory.db xem dữ liệu đã thực sự được lưu hay chưa.

class FunctionalMemoryEvaluator : public Evaluator {
private:
    int rewardScore_;

public:
    /**
     * @brief Khởi tạo Giám khảo Tính năng.
     * @param score Số điểm được cộng nếu phát hiện DB thay đổi đúng.
     */
    FunctionalMemoryEvaluator(int score = 50) : rewardScore_(score) {}

    std::string getName() const override {
        return "FunctionalMemoryEvaluator";
    }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& trajectory, Environment* env) override {
        // Mẹo Clean Code: Ta ép kiểu void finalAnswer và trajectory vì giám khảo này không cần đọc chúng
        (void)finalAnswer;
        (void)trajectory;

        // An toàn bộ nhớ: Kiểm tra xem có môi trường để chấm không
        if (!env) {
            std::cerr << "  [-] [Functional Eval] Loi: Khong co Environment de kiem tra hien truong!\n";
            return std::nullopt; // Báo lỗi Nullopt (Kỹ thuật C++17)
        }

        std::cout << "  -> [Functional Eval] Dang truy van vao SQLite de kiem tra bieu hien thuc te...\n";
        
        // Tạo câu lệnh SQL đếm số dòng có chữ "Ha Noi" trong file SQLite
        std::string sqlCmd = "sqlite3 agent_memory.db \"SELECT COUNT(*) FROM agent_history WHERE content LIKE '%Ha Noi%';\"";
        
        // Giao câu lệnh cho Environment thực thi và lấy kết quả trả về
        std::string result = env->executeCommand(sqlCmd);

        // Phân tích kết quả thực tế (Giả định trả về chuỗi "1" hoặc lớn hơn là có dữ liệu)
        if (result == "1" || result != "0") {
            std::cout << "  -> [Functional Eval] Phat hien du lieu trong DB! Cong diem.\n";
            return rewardScore_; // Trả về 50 điểm
        }

        std::cout << "  -> [Functional Eval] Khong tim thay du lieu trong DB.\n";
        return 0; // Trả về 0 điểm nếu DB trống rỗng
    }
};