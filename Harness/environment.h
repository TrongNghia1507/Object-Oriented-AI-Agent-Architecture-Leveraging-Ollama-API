#pragma once
#include <string>
#include <iostream>

// Lớp Gốc Trừu Tượng
class Environment {
public:
    virtual ~Environment() = default;

    // Chuẩn bị môi trường trước khi test
    virtual void setup() = 0;

    // Nhận lệnh từ Evaluator/Tool và thực thi (Trả về kết quả Terminal)
    virtual std::string executeCommand(const std::string& command) = 0;

    // Dọn dẹp sau khi test xong
    virtual void teardown() = 0;
};

// Lớp Cụ Thể: Chạy trực tiếp trên máy thật (Native)
class NativeEnvironment : public Environment {
public:
    void setup() override {
        std::cout << "[Environment] Dang thiet lap moi truong Native... (vd: Tao DB SQLite rong)\n";
        // Có thể thêm lệnh tạo file agent_memory.db rỗng tại đây
    }

    std::string executeCommand(const std::string& command) override {
        // Trong thực tế, bạn sẽ dùng popen() để lấy output từ terminal C++
        // Ở đây ta mô phỏng việc trả về kết quả
        std::cout << "[Environment Exec] " << command << "\n";
        return "1"; // Giả sử dòng lệnh SQL trả về số lượng row là 1
    }

    void teardown() override {
        std::cout << "[Environment] Dang don dep moi truong... (vd: Xoa DB SQLite tam)\n";
    }
};