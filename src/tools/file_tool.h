#pragma once
#include "tool.h"
#include "../utils/SystemUtils.h"
#include <string>
#include <memory>
#include <stdexcept>

class Environment;

// Dùng chung 1 class FileTool nhưng 2 "chế độ" (read/write) — tránh trùng lặp
// code Environment* giữa 2 tool riêng biệt. Đăng ký 2 lần với 2 tên khác nhau
// (xem cách dùng ở makeFileTools() bên dưới) để LLM thấy đúng 2 tool riêng
// "read_file" và "write_file" như đề bài yêu cầu.

class FileTool : public Tool {
public:
    enum class Mode { Read, Write };

private:
    Environment* env_;
    Mode mode_;

public:
    FileTool(Environment* env, Mode mode);

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& arguments) override;
};

// Hàm tiện ích: tạo cặp (read_file, write_file) dùng chung 1 Environment*,
// để nơi đăng ký tool (ví dụ tests/llm_agent_test.cpp) gọi gọn gàng thay vì
// tự viết lặp lại 2 lần.
std::unique_ptr<FileTool> makeReadFileTool(Environment* env);
std::unique_ptr<FileTool> makeWriteFileTool(Environment* env);