#include "exec_tool.h"
#include <iostream>
#include <stdexcept>

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

ExecTool::ExecTool(Environment* env, ExecutionPolicy policy) 
    : env_(env), policy_(std::move(policy)) {
    if (!env_) {
        throw std::runtime_error("Fatal Error: ExecTool can nhan mot con tro Environment hop le!");
    }
}

std::string ExecTool::execute(const std::string& arguments) {
    std::string command     = arguments;
    std::string explanation = "(khong co giai thich)";

    auto sep = arguments.find(" | ");
    if (sep != std::string::npos) {
        command     = arguments.substr(0, sep);
        explanation = arguments.substr(sep + 3);
    }
    
    // Làm sạch ký tự xuống dòng rác từ mô hình AI sinh ra
    while (!command.empty() && (command.back() == ' ' || command.back() == '\r' || command.back() == '\n'))
        command.pop_back();

    if (command.empty()) return "Loi: lenh rong.";

    if (!policy_(command, explanation)) {
        return "Da tu choi thuc thi lenh vi vi pham chinh sach bao mat: " + command;
    }

    return env_->executeCommand(command);
}