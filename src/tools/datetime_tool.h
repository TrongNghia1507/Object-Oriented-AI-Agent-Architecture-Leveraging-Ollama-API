#pragma once
#include "tool.h"
#include <string>

// DateTimeTool

class DateTimeTool : public Tool {
private:
    // Chuẩn hoá arguments: bỏ khoảng trắng thừa + hạ chữ thường để so khớp mode.
    static std::string normalize(const std::string& s);

public:
    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& arguments) override;
};