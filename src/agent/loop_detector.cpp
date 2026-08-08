#include "loop_detector.h"
#include <algorithm>
#include <sstream>

LoopDetector::LoopDetector(int warningThreshold, int criticalThreshold)
    : warningThreshold_(warningThreshold)
    , criticalThreshold_(criticalThreshold) {}

void LoopDetector::reset() { history_.clear(); }

LoopStatus LoopDetector::checkGenericRepeat(const ActionRecord& cur,
                                             std::string& outReason) const {
    // Đếm số lần cur đã xuất hiện trong history_ (TRƯỚC khi add)
    int count = static_cast<int>(std::count(history_.begin(), history_.end(), cur));
    // Sau khi add sẽ là count+1 lần tổng cộng
    if (count + 1 >= criticalThreshold_) {
        std::ostringstream os;
        os << "[LOOP/CRITICAL] Generic repeat: tool='" << cur.toolName
           << "' args='" << cur.arguments
           << "' da lap " << count + 1 << "/" << criticalThreshold_ << " lan.";
        outReason = os.str();
        return LoopStatus::Critical;
    }
    if (count + 1 >= warningThreshold_) {
        std::ostringstream os;
        os << "[LOOP/WARNING] Generic repeat: tool='" << cur.toolName
           << "' args='" << cur.arguments
           << "' da lap " << count + 1 << "/" << warningThreshold_ << " lan.";
        outReason = os.str();
        return LoopStatus::Warning;
    }
    return LoopStatus::Clear;
}

LoopStatus LoopDetector::checkPingPong(std::string& outReason) const {
    // Cần ít nhất 4 bước (đã add current) để phát hiện A-B-A-B
    if (history_.size() < 4) return LoopStatus::Clear;
    const auto& a1 = history_[history_.size() - 4];
    const auto& b1 = history_[history_.size() - 3];
    const auto& a2 = history_[history_.size() - 2];
    const auto& b2 = history_[history_.size() - 1];
    if (a1 == a2 && b1 == b2 && !(a1 == b1)) {
        std::ostringstream os;
        os << "[LOOP/CRITICAL] Ping-pong: ("
           << a1.toolName << "/" << a1.arguments << ") <-> ("
           << b1.toolName << "/" << b1.arguments << ") lap lai.";
        outReason = os.str();
        return LoopStatus::Critical;
    }
    return LoopStatus::Clear;
}

LoopStatus LoopDetector::check(const std::string& toolName,
                                const std::string& arguments,
                                std::string& outReason) {
    ActionRecord cur{toolName, arguments};

    // Kiểm tra generic repeat trước khi add vào history
    LoopStatus status = checkGenericRepeat(cur, outReason);

    // Add vào history (phải add trước khi kiểm tra ping-pong)
    history_.push_back(cur);

    // Critical từ generic repeat → dừng, không cần kiểm tra thêm
    if (status == LoopStatus::Critical) return status;

    // Kiểm tra ping-pong (dùng history đã bao gồm current)
    std::string ppReason;
    if (checkPingPong(ppReason) == LoopStatus::Critical) {
        outReason = ppReason;
        return LoopStatus::Critical;
    }

    return status; // Clear hoặc Warning
}
