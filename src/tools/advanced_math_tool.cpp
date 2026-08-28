#include "advanced_math_tool.h"
#include "exprtk.hpp" 
#include <algorithm>

std::string AdvancedMathTool::execute(const std::string& arguments) {
    // Làm sạch chuỗi đầu vào loại bỏ khoảng trắng và ký tự bao bọc chuỗi rác
    
    std::string args = arguments;
    std::string cleanArgs = args;
    cleanArgs.erase(std::remove(cleanArgs.begin(), cleanArgs.end(), '\"'), cleanArgs.end());

    typedef exprtk::symbol_table<double> symbol_table_t;
    typedef exprtk::expression<double>   expression_t;
    typedef exprtk::parser<double>       parser_t;

    symbol_table_t symbol_table;
    symbol_table.add_constants(); // Đăng ký tự động các hằng số toán học hệ thống như pi, e

    expression_t expression;
    expression.register_symbol_table(symbol_table);

    parser_t parser;
    if (!parser.compile(cleanArgs, expression)) {
        return "Error: Khong the phan tich cu phap toan nang cao.";
    }

    try {
        double result = expression.value();
        return std::to_string(result);
    } 
    catch (...) {
        return "Error: Loi tinh toan gia tri so thuc trong thu vien ExprTk.";
    }
}