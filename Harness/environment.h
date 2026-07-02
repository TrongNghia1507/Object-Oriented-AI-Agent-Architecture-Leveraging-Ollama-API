#pragma once
#include <string>

// ============================================================================
// INTERFACE LỚP GỐC TRỪU TƯỢNG (OS WRAPPER)
// ============================================================================
class Environment {
public:
    virtual ~Environment() = default;

    // Chuẩn bị hạ tầng môi trường trước khi kích hoạt tác vụ
    virtual void setup() = 0;

    // Nhận câu lệnh từ ExecTool và thực thi trực tiếp qua Terminal hệ điều hành
    virtual std::string executeCommand(const std::string& command) = 0;

    // Đọc tập tin an toàn theo cơ chế phân vùng của từng môi trường
    virtual std::string readFile(const std::string& path) = 0;

    // Ghi tập tin an toàn theo cơ chế phân vùng của từng môi trường
    virtual std::string writeFile(const std::string& path, const std::string& content) = 0;

    // Dọn dẹp môi trường hạ tầng sau khi bài test hoàn tất chu kỳ
    virtual void teardown() = 0;
};

// ============================================================================
// MÔI TRƯỜNG MÁY THẬT (NATIVE ENVIRONMENT)
// ============================================================================
class NativeEnvironment : public Environment {
public:
    void setup() override;
    std::string executeCommand(const std::string& command) override;
    std::string readFile(const std::string& path) override;
    std::string writeFile(const std::string& path, const std::string& content) override;
    void teardown() override;
};

// ============================================================================
// MÔI TRƯỜNG HỘP CÁT CÔ LẬP (SANDBOX ENVIRONMENT)
// ============================================================================
class SandboxEnvironment : public Environment {
private:
    std::string sandboxDir_;
public:
    // Mặc định tạo vùng cách ly tại folder "./sandbox_root" nếu không truyền tham số
    explicit SandboxEnvironment(std::string sandboxDir = "./sandbox_root");
    
    void setup() override;
    std::string executeCommand(const std::string& command) override;
    std::string readFile(const std::string& path) override;
    std::string writeFile(const std::string& path, const std::string& content) override;
    void teardown() override;
};