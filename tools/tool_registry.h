#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "tool.h"

class ToolRegistry {
private:
    // Kho lưu trữ: Map giữa "Tên Tool" và "Con trỏ thông minh quản lý đối tượng Tool"
    std::unordered_map<std::string, std::unique_ptr<Tool>> registry_;

public:
    ToolRegistry() = default;
    
    // Ngăn chặn việc sao chép vùng quản lý Registry để tránh xung đột con trỏ dữ liệu
    ToolRegistry(const ToolRegistry&) = delete;
    ToolRegistry& operator=(const ToolRegistry&) = delete;

    // Move OK (unique_ptr bên trong hỗ trợ move)
    ToolRegistry(ToolRegistry&&) = default;
    ToolRegistry& operator=(ToolRegistry&&) = default;

    // Đăng ký một công cụ mới vào hệ thống (Nhận vào một con trỏ unique_ptr)
    void registerTool(std::unique_ptr<Tool> tool);

    // Tìm kiếm và lấy ra con trỏ của Tool dựa vào tên định danh
    Tool* getTool(const std::string& name) const;

    // Trả về danh sách tên của tất cả các Tool đang có (Phục vụ cho việc tạo System Prompt gửi AI)
    std::vector<std::string> getAllToolNames() const;

    // Sinh ra chuỗi văn bản mô tả tổng hợp tất cả các tool để nạp vào nhận thức của Agent
    std::string generateToolsPrompt() const;
};

#endif // TOOL_REGISTRY_H