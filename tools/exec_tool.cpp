#include "exec_tool.h"

// Giữ lại xíu Macro cho popen để code compile mượt cả trên Windows lẫn Linux/Codespaces
#ifdef _WIN32
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif
std::string ExecTool::execute(const json& payload){
    // 1. Bóc tách lệnh bash
    std::string args = payload["arguments"].get<std::string>();
    
    // 2. Bóc tách lời giải thích (fallback nếu AI quên truyền)
    std::string explanation = "Không có giải thích chi tiết từ AI.";
    if (payload.contains("explanation")) {
        explanation = payload["explanation"].get<std::string>();
    }

    // 3. HIỂN THỊ CẢNH BÁO TRÊN TERMINAL
    std::cout << "\n==================================================\n";
    std::cout << "[!] CẢNH BÁO BẢO MẬT: AI muốn can thiệp hệ thống\n";
    std::cout << ">>> Lệnh kỹ thuật : " << args << "\n";
    std::cout << ">>> AI Giải thích : " << explanation << "\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << "Bạn có cho phép chạy lệnh này không? (y/n): ";
    
    // Nhận input từ bàn phím người dùng
    std::string choice;
    std::getline(std::cin, choice);

    // 4. Xử lý quyết định
    if (choice != "y" && choice != "Y") {
        std::cout << "[Hệ thống] Đã từ chối cấp quyền.\n";
        return "Error: User denied permission to execute the command: " + args;
    }

    std::cout << "[Hệ thống] Đang thực thi lệnh...\n";
    return runSystemCommand(args);
}
// Hàm thực thi lệnh gom stdout + stderr
std::string ExecTool::runSystemCommand(const std::string& cmd) {
    std::string cmd_with_stderr = cmd + " 2>&1";
    std::array<char, 128> buffer;
    std::string result;
    
    // Dùng Smart Pointer để tự động đóng ống pipe, chống tràn RAM
    std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(cmd_with_stderr.c_str(), "r"), PCLOSE);
    
    if (!pipe) {
        return "System Error: Không thể mở pipe để chạy lệnh.";
    }
    
    // Hút dữ liệu text từ Terminal ngầm
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    if (result.empty()) {
        return "Command executed successfully with no output.";
    }
    
    return result;
}
