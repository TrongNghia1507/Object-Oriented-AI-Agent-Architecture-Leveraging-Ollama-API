#ifndef CALCULATOR_TOOL_H
#define CALCULATOR_TOOL_H

#include <string>
#include <vector>
#include <stack>
#include "tool.h" // Interface lớp cơ sở Tool có sẵn trong đồ án của bạn

class CalculatorTool : public Tool {
private:
    // Các hàm trợ giúp xử lý nội bộ thuật toán Shunting-Yard
    int getPrecedence(char op);
    bool isOperator(char c);
    double applyOp(double a, double b, char op);
    std::vector<std::string> infixToPostfix(const std::string& tokens);
    double evaluatePostfix(const std::vector<std::string>& postfix);

public:
    std::string getName() const override { return "calculator"; }
    
    std::string getDescription() const override { 
        return "Giai quyet bieu thuc so hoc co ban (+, -, *, /) co dau ngoac long nhau. "
               "Tham so truyen vao phai la chuoi bieu thuc sach, vi du: '(15+5)*17'";
    }

    std::string execute(const json& payload) override;
};

#endif // CALCULATOR_TOOL_H