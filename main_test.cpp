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

    // 1. Khởi tạo Database Manager (Chỉ 1 kết nối duy nhất)
    auto dbManager = std::make_shared<DatabaseManager>();
    std::cout << "[+] Đã khởi tạo Database Manager và bảng agent_memory." << std::endl;

    // 2. Khởi tạo 2 tay chân (Công cụ)
    MemorySaveTool saveTool(dbManager);
    MemorySearchTool searchTool(dbManager);


    std::cout << "\n--- TEST 1: LƯU TRỮ DỮ LIỆU ---" << std::endl;
    std::string textToSave = "Kết quả phép tính 15 * 3 là 45";
    std::cout << "AI ra lệnh lưu: \"" << textToSave << "\"" << std::endl;
    
    std::string saveResult = saveTool.execute(textToSave);
    std::cout << "Phản hồi từ Tool: " << saveResult << std::endl;

 
    std::cout << "\n--- TEST 2: TÌM KIẾM CÓ DỮ LIỆU ---" << std::endl;
    std::string keyword1 = "15 * 3";
    std::cout << "AI ra lệnh tìm kiếm: \"" << keyword1 << "\"" << std::endl;
    
    std::string searchHit = searchTool.execute(keyword1);
    if (!searchHit.empty()) {
        std::cout << "=> THÀNH CÔNG! Tool tìm thấy: " << searchHit << std::endl;
    } else {
        std::cout << "=> THẤT BẠI! Lỗi logic, đáng lẽ phải tìm thấy." << std::endl;
    }


    std::cout << "\n--- TEST 3: TÌM KIẾM KHÔNG CÓ DỮ LIỆU ---" << std::endl;
    std::string keyword2 = "mật khẩu wifi";
    std::cout << "AI ra lệnh tìm kiếm: \"" << keyword2 << "\"" << std::endl;
    
    std::string searchMiss = searchTool.execute(keyword2);
    if (searchMiss.empty()) {
        std::cout << "=> THÀNH CÔNG! Trả về chuỗi rỗng đúng như dự kiến vì chưa từng lưu." << std::endl;
    } else {
        std::cout << "=> THẤT BẠI! Tool tìm ra dữ liệu sai." << std::endl;
    }

    std::cout << "\n=== KẾT THÚC KIỂM THỬ ===" << std::endl;
    return 0;
}