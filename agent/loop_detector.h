#pragma once
#include <string>
#include <vector>

// Mức độ loop (3.5: configurable warning vs critical)
enum class LoopStatus {
    Clear,    // bình thường
    Warning,  // lặp nhẹ — log cảnh báo nhưng tiếp tục
    Critical  // lặp nghiêm trọng — AgentLoop phải dừng
};

// LoopDetector phát hiện 2 loại loop theo yêu cầu 3.5:
//
//  1. Generic repeat: cùng (toolName, arguments) xuất hiện lần thứ N.
//     Warning khi lần thứ warningThreshold, Critical khi lần thứ criticalThreshold.
//
//  2. Ping-pong: 4 hành động gần nhất tạo mẫu A-B-A-B (A ≠ B).
//     Luôn trả về Critical ngay khi phát hiện.
//
// LoopDetector không biết AgentLoop, không biết LLM — chỉ nhận
// (toolName, arguments) từng bước và tự duy trì lịch sử nội bộ.
class LoopDetector {
private:
    struct ActionRecord {
        std::string toolName;
        std::string arguments;
        bool operator==(const ActionRecord& o) const {
            return toolName == o.toolName && arguments == o.arguments;
        }
    };

    std::vector<ActionRecord> history_;
    int warningThreshold_;
    int criticalThreshold_;

    LoopStatus checkGenericRepeat(const ActionRecord& cur, std::string& outReason) const;
    LoopStatus checkPingPong(std::string& outReason) const;

public:
    // warningThreshold: xuất hiện lần thứ N → Warning
    // criticalThreshold: xuất hiện lần thứ N → Critical (dừng agent)
    explicit LoopDetector(int warningThreshold = 2, int criticalThreshold = 3);

    // Gọi sau mỗi ToolCallAction. FinalAnswerAction không cần kiểm tra.
    // outReason: được gán thông báo mô tả loop (nếu không Clear).
    LoopStatus check(const std::string& toolName,
                     const std::string& arguments,
                     std::string& outReason);

    void reset();
    int historySize() const { return static_cast<int>(history_.size()); }
};
