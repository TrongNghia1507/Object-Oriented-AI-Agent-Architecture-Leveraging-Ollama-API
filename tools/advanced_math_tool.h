#ifndef ADVANCED_MATH_TOOL_H
#define ADVANCED_MATH_TOOL_H

#include <string>
#include "tool.h"

class AdvancedMathTool : public Tool {
public:
    std::string getName() const override { return "advanced_math"; }

    std::string getDescription() const override {
        return "Giai quyet cac phep toan cao cap va bieu thuc phuc tap chua "
               "sqrt, pow, ^, sin, cos, tan, log, pi, e. "
               "Tham so truyen vao phai la chuoi bieu thuc, vi du: 'sqrt(144) + 2^3'";
    }

    std::string execute(const std::string& arguments) override;
};

#endif // ADVANCED_MATH_TOOL_H