#pragma once
#include "../agent/agent_loop.h"
#include "../agent/trajectory.h"
#include "evaluator.h"
#include "../LLMClient/json.hpp"
#include <vector>
#include <memory>
#include <string>

// ─── HarnessRunner — Hệ thống điều phối và Đánh giá tự động ──────────────────
//
// DESIGN PATTERN: Đóng vai trò Client quản lý danh sách các Strategy (Evaluator).
//   Nhiệm vụ: Cấu hình bài test, tiêm hook nghe lén ngầm vào AgentLoop,
//   kích hoạt vòng lặp ReAct và thu thập kết quả chấm điểm độc lập.
//
// KỸ THUẬT C++17 SỬ DỤNG:
//   Sử dụng std::shared_ptr quản lý vùng nhớ đa hình tự động cho các bộ Evaluator.
//   Tiêm hàm ẩn danh (Lambda expression) bắt ngữ cảnh qua tham chiếu [&] vào Hook.

class HarnessRunner {
private:
    std::vector<std::shared_ptr<Evaluator>> evaluators_; // Bộ sưu tập các tiêu chí chấm điểm

public:
    HarnessRunner() = default;
    ~HarnessRunner() = default;

    /**
     * @brief Đăng ký thêm một tiêu chí chấm điểm mới vào hệ thống Harness.
     * @param evaluator Con trỏ thông minh trỏ tới lớp kế thừa của Evaluator.
     */
    void addEvaluator(std::shared_ptr<Evaluator> evaluator);

    /**
     * @brief Kích hoạt thực thi task, theo dõi ngầm và kết xuất báo cáo JSON.
     * @param taskId Mã định danh duy nhất của tác vụ (Dùng đặt tên file báo cáo).
     * @param task Câu lệnh Prompt yêu cầu đầu vào dành cho Agent.
     * @param agent Tham chiếu tới đối tượng AgentLoop chính.
     */
    void runTask(const std::string& taskId, const std::string& task, AgentLoop& agent);
};