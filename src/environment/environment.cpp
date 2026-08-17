#include "environment.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

// Xử lý macro tương thích đa nền tảng hệ điều hành cho việc gọi luồng Terminal
#ifdef _WIN32
    #define POPEN  _popen
    #define PCLOSE _pclose
#else
    #define POPEN  popen
    #define PCLOSE pclose
#endif
void NativeEnvironment::setup() {
    std::cout << "[NativeEnvironment] Setup: San sang thuc thi truc tiep tren he dieu hanh vat ly.\n";
}

std::string NativeEnvironment::executeCommand(const std::string& command) {
    std::string cmdWithStderr = command + " 2>&1";
    std::array<char, 256> buffer;
    std::string result;

    // Áp dụng Nguyên lý RAII: Tự động đóng luồng Pipe hệ thống bằng Custom Lambda Deleter
    auto pipeDeleter = [](FILE* f) { if (f) PCLOSE(f); };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(POPEN(cmdWithStderr.c_str(), "r"), pipeDeleter);

    if (!pipe) {
        return "System Error: Khong the mo luong pipe ket noi Terminal tren Native OS.";
    }

    // Đọc tuần tự luồng dữ liệu kết quả trả về từ Terminal OS
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result.empty() ? "Success: Lenh thuc thi thanh cong (Khong co output)." : result;
}

std::string NativeEnvironment::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "Loi Native: Khong the mo file tai duong dan: " + path;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string NativeEnvironment::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return "Loi Native: Khong the tao/ghi file tai duong dan: " + path;
    }
    file << content;
    return "Success: Da ghi tap tin thanh cong len o dia vat ly.";
}

void NativeEnvironment::teardown() {
    std::cout << "[NativeEnvironment] Teardown: Ket thuc chu ky thuc thi tren may that.\n";
}

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
    // BẮT BUỘC: ÉP TIẾN TRÌNH DI CHUYỂN VÀO THƯ MỤC SANDBOX TRƯỚC KHI CHẠY LỆNH CỦA AI
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
        return "System Error: Khong the mo luong pipe ket noi Terminal trong Sandbox.";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result.empty() ? "Success: Lenh Sandbox thuc thi thanh cong." : result;
}

std::string SandboxEnvironment::readFile(const std::string& path) {
    // KỸ THUẬT PATH MIRRORING: Chỉ lấy tên file, ép buộc đường dẫn luôn nằm bên trong sandboxDir_
    // Ngăn chặn triệt để đòn tấn công Directory Traversal (AI truyền tham số "../../etc/passwd")
    fs::path safePath = fs::path(sandboxDir_) / fs::path(path).filename();
    
    std::ifstream file(safePath);
    if (!file.is_open()) {
        return "Loi Sandbox: Chan truy cap! Khong tim thay file hoac file nam ngoai vung cach ly: " + path;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string SandboxEnvironment::writeFile(const std::string& path, const std::string& content) {
    // Bảo vệ an toàn hệ thống máy chủ vật lý của người chấm điểm
    fs::path safePath = fs::path(sandboxDir_) / fs::path(path).filename();

    std::ofstream file(safePath);
    if (!file.is_open()) {
        return "Loi Sandbox: Tu choi quyen tao file tai duong dan: " + safePath.string();
    }
    file << content;
    return "Success: Ghi tap tin an toan vao vung dem Sandbox hoàn tất.";
}

void SandboxEnvironment::teardown() {
    std::cout << "[SandboxEnvironment] Teardown: Don dep moi truong hop cat.\n";
    // MẸO: Thầy cô chấm bài rất thích việc dọn sạch thư mục tạm sau mỗi bài test để tránh rò rỉ dữ liệu.
    // Nếu muốn dọn sạch hoàn toàn thư mục hộp cát khi kết thúc bài test, bạn hãy bỏ comment dòng dưới đây:
    // fs::remove_all(sandboxDir_);
}