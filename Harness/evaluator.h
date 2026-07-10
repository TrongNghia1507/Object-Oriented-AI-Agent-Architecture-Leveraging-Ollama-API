#pragma once
#include "../agent/trajectory.h"
#include "environment.h" // Nhúng môi trường để Giám khảo kiểm tra
#include <string>
#include <optional>

class Evaluator {
public:
    virtual ~Evaluator() = default;

    // Lấy tên bộ chấm điểm để in ra log JSON
    virtual std::string getName() const = 0;

    // Hàm chấm điểm đa hình (Strategy Pattern)
    virtual std::optional<int> evaluate(
        const std::string& finalAnswer, 
        const Trajectory& trajectory, 
        Environment* env
    ) = 0;
    virtual bool isPassed() const = 0;
};