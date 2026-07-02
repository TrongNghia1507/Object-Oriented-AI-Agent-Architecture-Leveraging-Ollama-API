#pragma once
#include "evaluator.h"
#include <iostream>
#include <string>

class FunctionalMemoryEvaluator : public Evaluator {
private:
    int rewardScore_;
public:
    FunctionalMemoryEvaluator(int score = 50) : rewardScore_(score) {}

    std::string getName() const override { return "FunctionalMemoryEvaluator"; }

    std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& traj, Environment* env) override {
        // Mẹo C++: Ép kiểu void để tránh cảnh báo (Warning) biến không sử dụng
        (void)finalAnswer;
        (void)traj;

        if (!env) {
            std::cerr << "  [-] [Functional Eval] Loi: Khong co Environment!\n";
            return std::nullopt;
        }

        std::cout << "  -> [Functional Eval] Dang vao hien truong de check DB...\n";
        
        // Giám khảo gõ lệnh SQL trực tiếp, KHÔNG thông qua Tool
        std::string sqlCmd = "sqlite3 agent_memory.db \"SELECT COUNT(*) FROM agent_history WHERE content LIKE '%Ha Noi%';\"";
        
        // Gọi CỬA SAU của Environment (Sẽ chạy lệnh thật qua popen)
        std::string result = env->executeEvalCommand(sqlCmd);

        std::cout << "  -> [Functional Eval] Output OS tra ve: '" << result << "'\n";

        // Xử lý kết quả Terminal an toàn
        try {
            // Ép kiểu chuỗi Terminal sang số nguyên (Integer)
            int count = std::stoi(result);
            
            if (count > 0) {
                std::cout << "  -> [Functional Eval] Phat hien " << count << " dong du lieu hop le! Cong diem.\n";
                return rewardScore_; // Agent đã lưu thật, cộng điểm!
            }
        } catch (...) {
            // Nếu result trả về rỗng "", hoặc chuỗi báo lỗi (không thể parse thành số)
            std::cout << "  -> [Functional Eval] Khong the doc duoc du lieu hoac DB trong rong.\n";
        }

        return 0; // DB trống rỗng hoặc bị lỗi Terminal, 0 điểm
    }
};