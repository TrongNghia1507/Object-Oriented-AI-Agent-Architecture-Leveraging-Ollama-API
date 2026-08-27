#pragma once
#include "tool.h"
#include <unordered_map>
#include <unordered_set> 
#include <string>
#include <memory>
#include <vector>

class ToolRegistry {
private:
    std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
    
    // TOOL POLICY: Lưu trữ danh sách tên các công cụ bị cấm
    std::unordered_set<std::string> blacklist_; 

public:
    ToolRegistry() = default;

    // Hàm đăng ký công cụ vào hệ thống
    void registerTool(std::unique_ptr<Tool> tool);

    // Quản lý chính sách Tool Policy 
    void banTool(const std::string& toolName);   // Thêm tool vào danh sách cấm
    void unbanTool(const std::string& toolName); // Gỡ lệnh cấm cho tool
    bool isAllowed(const std::string& toolName) const; // Kiểm tra chính sách cho phép

    // Hàm truy xuất công cụ (Sẽ tích hợp kiểm tra Tool Policy)
    Tool* getTool(const std::string& name) const;
    
    const std::unordered_map<std::string, std::unique_ptr<Tool>>& getAllTools() const;

    // Trả về danh sách tên tất cả tool đã đăng ký (kể cả tool đang bị blacklist,
    // vì AgentLoop dùng hàm này chỉ để báo lỗi "tool X không tồn tại, các tool có: ...").
    std::vector<std::string> getAllToolNames() const;

    // Sinh đoạn text mô tả toàn bộ tool (name + description) để nhét vào system
    // prompt cho LLM đọc. Bỏ qua các
    // tool đang bị banTool() để LLM không thấy/không gọi được tool đã cấm.
    std::string generateToolsPrompt() const;
};