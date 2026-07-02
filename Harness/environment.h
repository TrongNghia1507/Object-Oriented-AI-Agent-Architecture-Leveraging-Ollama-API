#pragma once
#include "../tools/tool.h"
#include <string>
#include <iostream>
#include <array>
#include <memory>
#include <stdexcept>

// --- Kỹ thuật hỗ trợ Đa nền tảng (Cross-platform) ---
#ifdef _WIN32
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif

// ─── Interface Gốc: Hệ thống quản lý Môi trường ───────────────────────────
class Environment {
public:
    virtual ~Environment() = default;
    virtual void setup() = 0;
    virtual void teardown() = 0;

    // 🚪 CỬA TRƯỚC: Dành cho AgentLoop thực thi công cụ (Bị kiểm soát)
    virtual std::string runTool(Tool* tool, const std::string& args) = 0;

    // 🚪 CỬA SAU (God Mode): Dành cho Harness/Evaluator chấm điểm
    virtual std::string executeEvalCommand(const std::string& command) = 0; 
};

// ─── Triển khai 1: Môi trường máy thật (Native) ───────────────────────────
class NativeEnvironment : public Environment {
public:
    void setup() override { 
        std::cout << "[Native Env] Setup sa hinh truc tiep tren OS that...\n"; 
    }
    void teardown() override { 
        std::cout << "[Native Env] Teardown, don dep hien truong may that.\n"; 
    }

    std::string runTool(Tool* tool, const std::string& args) override {
        if (!tool) return "Error: Tool rỗng";
        std::cout << "[Native Env] Cho phep '" << tool->getName() << "' chay truc tiep.\n";
        return tool->execute(args);
    }

    std::string executeEvalCommand(const std::string& command) override {
    std::cout << "[God Mode - Eval] Dang chay lenh OS thuc te: " << command << "\n";
    
    std::array<char, 128> buffer;
    std::string result;
    
    // Mở pipe để chạy lệnh CMD/Bash và đọc Output (stdout)
    std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(command.c_str(), "r"), PCLOSE);
    
    if (!pipe) {
        std::cerr << "[Eval Error] Khong the mo popen() cho lenh: " << command << "\n";
        return ""; // Trả về chuỗi rỗng nếu lỗi
    }
    
    // Đọc từng dòng kết quả từ terminal đổ vào biến result
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    // Xóa ký tự xuống dòng '\n' ở cuối (nếu có) để Evaluator dễ so sánh chuỗi (== "1")
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}
};

// ─── Triển khai 2: Môi trường Hộp cát cách ly (Sandbox / Docker) ──────────
class SandboxEnvironment : public Environment {
public:
    void setup() override { 
        std::cout << "[Sandbox Env] Dang khoi tao Docker Container 'agent_sandbox'...\n"; 
    }
    void teardown() override { 
        std::cout << "[Sandbox Env] Dang huy va xoa Container 'agent_sandbox' de xoa dau vet.\n"; 
    }

    std::string runTool(Tool* tool, const std::string& args) override {
        if (!tool) return "Error: Tool rỗng";
        
        std::cout << "[Sandbox Env] CHAN BAT hanh dong. Ep Tool '" 
                  << tool->getName() << "' phai thuc thi ben trong o dia lanh net Docker.\n";
        
        // Trong thuc te, lenh tuong tac (vi du ghi file note.txt) se duoc convert thanh:
        // "docker exec agent_sandbox echo '" + args + "' > note.txt"
        // O day ta goi tool->execute nhu mot phuong thuc bọc cach ly vung nho
        return "[Docker Sandbox Output] -> " + tool->execute(args);
    }

   std::string executeEvalCommand(const std::string& command) override {
        // [QUAN TRỌNG]: Biến lệnh kiểm tra bình thường thành lệnh chui vào Docker
        std::string sandboxCmd = "docker exec agent_sandbox " + command;
        
        std::cout << "[God Mode - Sandbox] Dang chui vao Docker chay lenh: " << sandboxCmd << "\n";
        
        std::array<char, 128> buffer;
        std::string result;
        
        // Chạy lệnh đã được bọc Docker
        std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(sandboxCmd.c_str(), "r"), PCLOSE);
        
        if (!pipe) {
            std::cerr << "[Eval Error] Khong the mo popen() cho lenh: " << sandboxCmd << "\n";
            return ""; 
        }
        
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        
        return result;
    }
};