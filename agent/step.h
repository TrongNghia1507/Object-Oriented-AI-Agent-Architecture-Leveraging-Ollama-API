#pragma once
#include <optional>
#include <string>
#include <variant>

// ─── Kiểu hành động — std::variant (C++17 bắt buộc) ─────────────────────────
// AgentLoop dùng std::visit + if constexpr để xử lý từng nhánh rõ ràng,
// không cần dynamic_cast hay if-else theo string.

struct ToolCallAction {
    std::string toolName;   // tên tool trong ToolRegistry (vd: "calculator")
    std::string arguments;  // chuỗi tham số nguyên văn từ model
};

struct FinalAnswerAction {
    std::string text;       // câu trả lời cuối cùng, không cần gọi thêm tool
};

// Action = ToolCallAction | FinalAnswerAction
using Action = std::variant<ToolCallAction, FinalAnswerAction>;

// ─── Step: 1 bước trong vòng lặp ReAct ───────────────────────────────────────
// Data class thuần — AgentLoop tạo, Trajectory lưu, HarnessRunner đọc.
// Không có dependency ngược: Step không biết ai tạo ra nó.
struct Step {
    int stepId = 0;

    std::string thought;    // toàn bộ raw text từ model (Thought + Action + Arguments)
    Action action;          // kết quả parse (ToolCallAction hoặc FinalAnswerAction)

    // std::optional (C++17): empty nếu là FinalAnswerAction,
    // có giá trị nếu tool đã được gọi và trả về kết quả.
    std::optional<std::string> toolResult;

    int tokensUsed = 0;
    long long latencyMs = 0;
};
