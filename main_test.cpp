#include <iostream>
#include <memory>
#include "tools/tool.h"
#include "tools/calculator_tool.h"
#include "tools/advanced_math_tool.h"
#include "tools/memory_tool.h"

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


    std::cout << "=== BẮT ĐẦU KIỂM THỬ MEMORY TOOL ===" << std::endl;

    // 1. Khởi tạo Database Manager 
    auto dbManager = std::make_shared<DatabaseManager>();
    std::cout << "[+] Đã khởi tạo Database Manager và bảng agent_memory." << std::endl;

    // 2. Khởi tạo 2 tay chân (Công cụ)
    MemorySaveTool saveTool(dbManager);
    MemorySearchTool searchTool(dbManager);


    std::cout << "\n--- TEST 1: LƯU TRỮ DỮ LIỆU CÓ TAG ---" << std::endl;
    // Thử đóng giả LLM truyền vào format có tag
    std::string textToSave1 = "[math_result] | Kết quả phép tính 15 * 3 là 45";
    std::cout << "AI ra lệnh lưu: \"" << textToSave1 << "\"" << std::endl;
    std::cout << "Phản hồi: " << saveTool.execute(textToSave1) << std::endl;

    std::string textToSave2 = "[scenario] | Một tình huống thực tế: Đi siêu thị mua 45 quả táo";
    std::cout << "AI ra lệnh lưu: \"" << textToSave2 << "\"" << std::endl;
    std::cout << "Phản hồi: " << saveTool.execute(textToSave2) << std::endl;

    std::cout << "\n--- TEST 2: TÌM KIẾM THEO BỐI CẢNH ---" << std::endl;
    std::string keyword1 = "15 * 3";
    std::cout << "AI tìm kiếm: \"" << keyword1 << "\"" << std::endl;
    std::cout << "=> KẾT QUẢ:\n" << searchTool.execute(keyword1) << std::endl;
    return 0;
}