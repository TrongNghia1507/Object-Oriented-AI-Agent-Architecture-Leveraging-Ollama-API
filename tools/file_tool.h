#pragma once
#include "tool.h"
#include <string>
#include <memory>
#include <stdexcept>

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
    FileTool(Environment* env, Mode mode) : env_(env), mode_(mode) {
        if (!env_) {
            throw std::runtime_error("Fatal Error: FileTool can nhan mot con tro Environment hop le!");
        }
    }

    std::string getName() const override {
        return mode_ == Mode::Read ? "read_file" : "write_file";
    }

    std::string getDescription() const override {
        if (mode_ == Mode::Read) {
            return "Doc noi dung 1 file van ban. Tham so: duong dan file, vi du: 'result.txt'.";
        }
        return "Ghi noi dung vao 1 file van ban (ghi de neu da ton tai). "
               "Tham so bat buoc dung format: 'duong_dan | noi_dung', "
               "vi du: 'result.txt | 255'.";
    }

    std::string execute(const std::string& arguments) override {
        if (mode_ == Mode::Read) {
            if (arguments.empty()) return "Loi: thieu duong dan file can doc.";
            return env_->readFile(arguments);
        }

        // Mode::Write — tách "path | content" giống format cua MemorySaveTool
        // ([TAG] | NOI DUNG) de nhat quan trong toan bo project.
        auto sep = arguments.find(" | ");
        if (sep == std::string::npos) {
            return "Loi: sai dinh dang. Can 'duong_dan | noi_dung'.";
        }
        std::string path = arguments.substr(0, sep);
        std::string content = arguments.substr(sep + 3);
        return env_->writeFile(path, content);
    }
};

// Hàm tiện ích: tạo cặp (read_file, write_file) dùng chung 1 Environment*,
// để nơi đăng ký tool (ví dụ tests/llm_agent_test.cpp) gọi gọn gàng thay vì
// tự viết lặp lại 2 lần.
inline std::unique_ptr<FileTool> makeReadFileTool(Environment* env) {
    return std::make_unique<FileTool>(env, FileTool::Mode::Read);
}
inline std::unique_ptr<FileTool> makeWriteFileTool(Environment* env) {
    return std::make_unique<FileTool>(env, FileTool::Mode::Write);
}
