#include "calculator_tool.h"
#include <sstream>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <stack>

int CalculatorTool::getPrecedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

bool CalculatorTool::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

double CalculatorTool::applyOp(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b == 0) throw std::runtime_error("Error: Chia cho 0");
            return a / b;
    }
    return 0;
}

std::vector<std::string> CalculatorTool::infixToPostfix(const std::string& expr) {
    std::vector<std::string> postfix;
    std::stack<char> opStack;
    size_t i = 0;

    while (i < expr.length()) {
        if (std::isspace(expr[i])) {
            i++;
            continue;
        }

        // Xử lý đọc số thực (bao gồm cả số thập phân)
        if (std::isdigit(expr[i]) || expr[i] == '.') {
            std::string num;
            while (i < expr.length() && (std::isdigit(expr[i]) || expr[i] == '.')) {
                num += expr[i];
                i++;
            }
            postfix.push_back(num);
            continue;
        }

        // Bẫy kỹ thuật xử lý số âm biệt lập hoặc đứng sau dấu ngoặc mở: ví dụ (-2 + 4)
        if (expr[i] == '-' && (i == 0 || expr[i - 1] == '(')) {
            postfix.push_back("0"); // Chèn số 0 kỹ thuật để biến phép trừ thành (0 - x)
        }

        if (expr[i] == '(') {
            opStack.push(expr[i]);
        } 
        else if (expr[i] == ')') {
            while (!opStack.empty() && opStack.top() != '(') {
                postfix.push_back(std::string(1, opStack.top()));
                opStack.pop();
            }
            if (opStack.empty()) throw std::runtime_error("Error: Sai cuphap ngoac");
            opStack.pop(); // Loại bỏ dấu '(' ra khỏi stack
        } 
        else if (isOperator(expr[i])) {
            while (!opStack.empty() && getPrecedence(opStack.top()) >= getPrecedence(expr[i])) {
                postfix.push_back(std::string(1, opStack.top()));
                opStack.pop();
            }
            opStack.push(expr[i]);
        } 
        else {
            throw std::runtime_error("Error: Ky tu khong hop le '" + std::string(1, expr[i]) + "'");
        }
        i++;
    }

    while (!opStack.empty()) {
        if (opStack.top() == '(' || opStack.top() == ')') 
            throw std::runtime_error("Error: Sai cuphap ngoac");
        postfix.push_back(std::string(1, opStack.top()));
        opStack.pop();
    }

    return postfix;
}

double CalculatorTool::evaluatePostfix(const std::vector<std::string>& postfix) {
    std::stack<double> valStack;

    for (const std::string& token : postfix) {
        if (token.length() > 1 || std::isdigit(token[0]) || token == ".") {
            valStack.push(std::stod(token));
        } else if (isOperator(token[0])) {
            if (valStack.size() < 2) throw std::runtime_error("Error: Thieu toan hang");
            double val2 = valStack.top(); valStack.pop();
            double val1 = valStack.top(); valStack.pop();
            valStack.push(applyOp(val1, val2, token[0]));
        }
    }

    if (valStack.size() != 1) throw std::runtime_error("Error: Bieu thuc khong hop le");
    return valStack.top();
}

std::string CalculatorTool::execute(const std::string& arguments) {
    try {
        std::string clean = arguments;
        clean.erase(std::remove(clean.begin(), clean.end(), '"'), clean.end());
        size_t s = clean.find_first_not_of(" \t\r\n");
        if (s == std::string::npos) return "Loi: bieu thuc rong";
        clean = clean.substr(s, clean.find_last_not_of(" \t\r\n") - s + 1);
        std::vector<std::string> postfix = infixToPostfix(clean);
        double result = evaluatePostfix(postfix);
        std::ostringstream ss;
        ss << result;
        return ss.str();
    } catch (const std::exception& e) {
        return std::string("Loi: ") + e.what();
    }
}
