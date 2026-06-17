#include <iostream>
#include <memory>
#include "tools/calculator_tool.h"
#include "tools/advanced_math_tool.h"

int main() {
    std::cout << "=== HE THONG KIEM THU CUON CHIEU - BOPHAT TOAN TOOL ===" << std::endl;

    // 1. Kiểm thử Máy tính toán cơ bản (Shunting-Yard)
    std::unique_ptr<Tool> basicCalc = std::make_unique<CalculatorTool>();
    std::cout << "\n[Test 1] Tool: " << basicCalc->getName() << std::endl;
    
    std::string exp1 = "(15 + 5) * 17";
    std::cout << "Input: " << exp1 << " -> Output: " << basicCalc->execute(exp1) << " (Ky vong: 340)" << std::endl;

    std::string exp2 = "-5.5 + (10 * 2)";
    std::cout << "Input: " << exp2 << " -> Output: " << basicCalc->execute(exp2) << " (Ky vong: 14.5)" << std::endl;
    
    std::string exp3 = "10 / 0";
    std::cout << "Input: " << exp3 << " -> Output: " << basicCalc->execute(exp3) << " (Ky vong: Error: Chia cho 0)" << std::endl;

    // 2. Kiểm thử Máy tính nâng cao (ExprTk Third-party)
    std::unique_ptr<Tool> advCalc = std::make_unique<AdvancedMathTool>();
    std::cout << "\n[Test 2] Tool: " << advCalc->getName() << std::endl;

    std::string advExp1 = "sqrt(144) + 2^3";
    std::cout << "Input: " << advExp1 << " -> Output: " << advCalc->execute(advExp1) << " (Ky vong: 20.000000)" << std::endl;

    std::string advExp2 = "sin(pi / 2) * 5";
    std::cout << "Input: " << advExp2 << " -> Output: " << advCalc->execute(advExp2) << " (Ky vong: 5.000000)" << std::endl;

    std::cout << "\n=======================================================" << std::endl;
    return 0;
}