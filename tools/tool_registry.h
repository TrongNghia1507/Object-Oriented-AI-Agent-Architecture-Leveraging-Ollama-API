#pragma once
#include "tool.h"
#include <unordered_map>
#include <unordered_set> // Thêm thư viện để làm danh sách Blacklist
#include <string>
#include <memory>

class ToolRegistry {
private:
    std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
    
    // HIỆN THỰC TOOL POLICY: Lưu trữ danh sách tên các công cụ bị cấm
    std::unordered_set<std::string> blacklist_; 

public:
    ToolRegistry() = default;

    // Hàm đăng ký công cụ vào hệ thống
    void registerTool(std::unique_ptr<Tool> tool);

    // CÁC HÀM MỚI: Quản lý chính sách Tool Policy theo yêu cầu đồ án
    void banTool(const std::string& toolName);   // Thêm tool vào danh sách cấm
    void unbanTool(const std::string& toolName); // Gỡ lệnh cấm cho tool
    bool isAllowed(const std::string& toolName) const; // Kiểm tra chính sách cho phép

    // Hàm truy xuất công cụ (Sẽ tích hợp kiểm tra Tool Policy)
    Tool* getTool(const std::string& name) const;
    
    const std::unordered_map<std::string, std::unique_ptr<Tool>>& getAllTools() const;
};