#pragma once
#include "../agent/trajectory.h" // Đường dẫn chính xác tới file trajectory.h trong thư mục agent
#include "environment.h"         // === THÊM MỚI: Nhúng interface Environment ===
#include <string>
#include <optional>              // Kỹ thuật C++17 bắt buộc theo yêu cầu đồ án

// ─── Evaluator Interface — Tầng đánh giá tự động (yêu cầu nâng cao) ──────────
//
// DESIGN PATTERN: Strategy Pattern
//   Hệ thống phân cấp bộ chấm điểm (KeywordEvaluator, FunctionalEvaluator, v.v.)
//   đều dùng chung một interface trừu tượng trừu gốc này.
//   HarnessRunner sẽ quản lý một danh sách các đa hình của lớp này để chấm điểm độc lập.
//
// KỸ THUẬT C++17 SỬ DỤNG:
//   std::optional<int> -> Cho phép trả về giá trị trống (nullopt) nếu một bộ chấm điểm
//                         gặp lỗi hoặc chưa đủ dữ kiện để đánh giá toàn diện bài test.

class Evaluator {
public:
    // Virtual destructor bắt buộc phải có cho lớp cơ sở đa hình (Tránh rò rỉ bộ nhớ)
    virtual ~Evaluator() = default;

    /**
     * @brief Lấy tên định danh của bộ đánh giá cụ thể.
     * @return std::string Tên bộ chấm điểm (Ví dụ: "KeywordEvaluator").
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Hàm ảo thuần túy xử lý logic chấm điểm bài test một cách kín đáo.
     * @param finalAnswer Câu trả lời cuối cùng thu được từ AgentLoop.
     * @param trajectory Toàn bộ nhật ký hành trình ReAct (Các bước đi, hành động) của Agent.
     * @param env Con trỏ trỏ tới Môi trường thực thi để Giám khảo kiểm tra vật chứng thực tế (ví dụ: test DB SQLite).
     * @return std::optional<int> Điểm số cục bộ (partial score). Trả về std::nullopt nếu không chấm được.
     */
    // === SỬA ĐỔI: Thêm tham số Environment* env ===
    virtual std::optional<int> evaluate(const std::string& finalAnswer, const Trajectory& trajectory, Environment* env) = 0;
};