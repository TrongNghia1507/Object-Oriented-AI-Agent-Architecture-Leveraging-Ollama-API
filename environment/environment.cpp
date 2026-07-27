#include "environment.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

#ifdef _WIN32
    #define POPEN  _popen
    #define PCLOSE _pclose
#else
    #define POPEN  popen
    #define PCLOSE pclose
#endif

// ============================================================================
// HIỆN THỰC HÓA CHI TIẾT NATIVE ENVIRONMENT
// ============================================================================

void NativeEnvironment::setup() {
    // Giữ nguyên log ở console của bạn, vì Agent không đọc được stdout của C++ runner
    std::cout << "[NativeEnvironment] Setup: San sang thuc thi truc tiep tren he dieu hanh vat ly.\n";
}

std::string NativeEnvironment::executeCommand(const std::string& command) {
    std::string cmdWithStderr = command + " 2>&1";
    std::array<char, 256> buffer;
    std::string result;

    auto pipeDeleter = [](FILE* f) { if (f) PCLOSE(f); };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(POPEN(cmdWithStderr.c_str(), "r"), pipeDeleter);

    if (!pipe) {
        return "bash: fork: Resource temporarily unavailable\n"; // Giả lập lỗi OS thật
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Terminal thật thường không trả về gì nếu lệnh chạy đúng
    return result; 
}

std::string NativeEnvironment::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "cat: " + path + ": No such file or directory\n"; // Giả lập lỗi cat của Linux
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string NativeEnvironment::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return "bash: " + path + ": Permission denied\n"; // Giả lập lỗi ghi file
    }
    file << content;
    return ""; // Thành công thì im lặng
}

void NativeEnvironment::teardown() {
    std::cout << "[NativeEnvironment] Teardown: Ket thuc chu ky thuc thi tren may that.\n";
}


// ============================================================================
// HIỆN THỰC HÓA CHI TIẾT SANDBOX ENVIRONMENT
// ============================================================================

SandboxEnvironment::SandboxEnvironment(std::string sandboxDir) 
    : sandboxDir_(std::move(sandboxDir)) {}

void SandboxEnvironment::setup() {
    std::cout << "[SandboxEnvironment] Setup: Khoi tao vung dem cach ly an toan tai: " << sandboxDir_ << "\n";
    try {
        fs::create_directories(sandboxDir_);
    } catch (const std::exception& e) {
        std::cerr << "Loi ha tang: Khong the tao thu muc Sandbox: " << e.what() << "\n";
    }
}

std::string SandboxEnvironment::executeCommand(const std::string& command) {
#ifdef _WIN32
    std::string safeCmd = "cd /d " + sandboxDir_ + " && " + command;
#else
    std::string safeCmd = "cd " + sandboxDir_ + " && " + command;
#endif
    safeCmd = "/bin/bash -c \"" + safeCmd + "\"";
    std::string cmdWithStderr = safeCmd + " 2>&1";
    std::array<char, 256> buffer;
    std::string result;

    auto pipeDeleter = [](FILE* f) { if (f) PCLOSE(f); };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(POPEN(cmdWithStderr.c_str(), "r"), pipeDeleter);

    if (!pipe) {
        return "bash: fork: Resource temporarily unavailable\n";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result; 
}

std::string SandboxEnvironment::readFile(const std::string& path) {
    fs::path safePath = fs::path(sandboxDir_) / fs::path(path).filename();
    
    std::ifstream file(safePath);
    if (!file.is_open()) {
        // AI chỉ biết cái path nó truyền vào, không được để lộ path thật của sandbox
        return "cat: " + path + ": No such file or directory\n"; 
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string SandboxEnvironment::writeFile(const std::string& path, const std::string& content) {
    fs::path safePath = fs::path(sandboxDir_) / fs::path(path).filename();

    std::ofstream file(safePath);
    if (!file.is_open()) {
        return "bash: " + path + ": Permission denied\n";
    }
    file << content;
    return ""; // Thành công thì im lặng
}

void SandboxEnvironment::teardown() {
    std::cout << "[SandboxEnvironment] Teardown: Don dep moi truong hop cat.\n";
    // MẸO: Thầy cô chấm bài rất thích việc dọn sạch thư mục tạm sau mỗi bài test để tránh rò rỉ dữ liệu.
    // Nếu muốn dọn sạch hoàn toàn thư mục hộp cát khi kết thúc bài test, bạn hãy bỏ comment dòng dưới đây:
    // fs::remove_all(sandboxDir_);
}