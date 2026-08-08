#pragma once
#include <string>
#include <vector>
#include <stack>
#include "tool.h"

class CalculatorTool : public Tool {
private:
    int getPrecedence(char op);
    bool isOperator(char c);
    double applyOp(double a, double b, char op);
    std::vector<std::string> infixToPostfix(const std::string& tokens);
    double evaluatePostfix(const std::vector<std::string>& postfix);

public:
    std::string getName() const override { return "calculator"; }
    std::string getDescription() const override {
        return "Giai bieu thuc so hoc co ban (+,-,*,/) co dau ngoac. "
               "Tham so: chuoi bieu thuc sach, vi du: '(15+5)*17'";
    }
    std::string execute(const std::string& arguments) override;
};
