#include "exec_tool.h"
#include <iostream>
#include <cstdio>

#ifdef _WIN32
    #define POPEN  _popen
    #define PCLOSE _pclose
#else
    #define POPEN  popen
    #define PCLOSE pclose
#endif

// ─── Policy: AskUser (định nghĩa ở đây để tránh iostream trong header) ───────
ExecutionPolicy ExecPolicy::AskUser() {
    return [](const std::string& command, const std::string& explanation) -> bool {
        std::cout << "\n==================================================\n"
                  << "[!] CANH BAO BAO MAT: AI muon can thiep he thong\n"
                  << ">>> Lenh       : " << command << "\n"
                  << ">>> Giai thich : " << explanation << "\n"
                  << "--------------------------------------------------\n"
                  << "Ban co cho phep khong? (y/n): ";
        std::string choice;
        std::getline(std::cin, choice);
        bool allowed = (choice == "y" || choice == "Y");
        if (!allowed) std::cout << "[He thong] Da tu choi.\n";
        return allowed;
    };
}

// ─── Constructor ──────────────────────────────────────────────────────────────
ExecTool::ExecTool(ExecutionPolicy policy) : policy_(std::move(policy)) {}

// ─── execute(): nhận chuỗi lệnh thuần, không nhận JSON ──────────────────────
// arguments = chuỗi lệnh shell (vd: "ls -la results/").
// Nếu model gửi thêm explanation theo format "CMD | EXPLAIN", bóc tách ra.
std::string ExecTool::execute(const std::string& arguments) {
    std::string command     = arguments;
    std::string explanation = "(khong co giai thich)";

    // Format tùy chọn: "ls -la | Liet ke file ket qua"
    auto sep = arguments.find(" | ");
    if (sep != std::string::npos) {
        command     = arguments.substr(0, sep);
        explanation = arguments.substr(sep + 3);
    }
    // Trim
    while (!command.empty() && (command.back() == ' ' || command.back() == '\r'))
        command.pop_back();

    if (command.empty()) return "Loi: lenh rong.";

    // Delegate cho policy — không tự quyết định
    if (!policy_(command, explanation)) {
        return "Da tu choi thuc thi lenh: " + command;
    }

    return runSystemCommand(command);
}

// ─── runSystemCommand ────────────────────────────────────────────────────────
std::string ExecTool::runSystemCommand(const std::string& cmd) {
    std::string cmdWithStderr = cmd + " 2>&1";
    std::array<char, 256> buffer;
    std::string result;

    // unique_ptr với custom deleter (C++17) — không leak FILE*
    auto pipeDeleter = [](FILE* f) { PCLOSE(f); };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(
        POPEN(cmdWithStderr.c_str(), "r"), pipeDeleter);

    if (!pipe) return "System Error: Khong the mo pipe.";

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
        if (result.size() > 4096) {   // giới hạn output, tránh tràn context
            result += "\n[...output bi cat ngan qua 4096 bytes...]";
            break;
        }
    }
    return result.empty() ? "Lenh chay thanh cong, khong co output." : result;
}
