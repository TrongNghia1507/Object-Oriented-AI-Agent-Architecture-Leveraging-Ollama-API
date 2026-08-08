#pragma once
#include "tool.h"
#include <string>

// DateTimeTool
// Không phụ thuộc Environment/LLMClient — tool "thuần" nhất trong hệ thống,
// nên không cần constructor injection như ExecTool/FileTool/VisionTool.
//
// Kỹ thuật C++20 sử dụng: std::chrono::zoned_time + std::format

class DateTimeTool : public Tool {
private:
    // Chuẩn hoá arguments: bỏ khoảng trắng thừa + hạ chữ thường để so khớp mode.
    static std::string normalize(const std::string& s);

public:
    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& arguments) override;
};