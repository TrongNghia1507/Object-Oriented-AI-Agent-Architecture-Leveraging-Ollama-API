#include "tool_registry.h"
#include <iostream>

void ToolRegistry::registerTool(std::unique_ptr<Tool> tool) {
    if (tool) {
        tools_[tool->getName()] = std::move(tool);
    }
}

// Thêm một tên công cụ vào danh sách đen (Banned list)
void ToolRegistry::banTool(const std::string& toolName) {
    blacklist_.insert(toolName);
    std::cout << "[ToolPolicy] Da thiet lap cam cong cu: " << toolName << "\n";
}

// Loại bỏ công cụ khỏi danh sách đen
void ToolRegistry::unbanTool(const std::string& toolName) {
    blacklist_.erase(toolName);
    std::cout << "[ToolPolicy] Da go lenh cam cho cong cu: " << toolName << "\n";
}

// Kiểm tra xem công cụ này có vi phạm chính sách bảo mật không
bool ToolRegistry::isAllowed(const std::string& toolName) const {
    // Nếu tên công cụ nằm trong blacklist_, trả về false (Denied)
    return blacklist_.find(toolName) == blacklist_.end();
}

// SỬA LỖI TRUY XUẤT: Nếu Agent cố tình gọi Tool bị cấm, trả về nullptr để chặn thực thi
Tool* ToolRegistry::getTool(const std::string& name) const {
    // 1. Kiểm tra Tool Policy trước
    if (!isAllowed(name)) {
        std::cerr << "[ToolPolicy CANH BAO]: Agent co tinh goi cong cu dang bi cam: " << name << "\n";
        return nullptr; // Trả về con trỏ rỗng để AgentLoop biết và báo lỗi cho LLM
    }

    // 2. Nếu an toàn, tiến hành tìm kiếm trong kho lưu trữ
    auto it = tools_.find(name);
    if (it != tools_.end()) {
        return it->second.get();
    }
    return nullptr;
}

// === getAllToolNames() — AgentLoop::doAct() gọi khi tool không tồn tại,
// để liệt kê cho LLM biết các tool nào đang có sẵn. ===
std::vector<std::string> ToolRegistry::getAllToolNames() const {
    std::vector<std::string> names;
    names.reserve(tools_.size());
    for (const auto& [name, tool] : tools_) {
        names.push_back(name);
    }
    return names;
}

// === generateToolsPrompt() — AgentLoop::buildSystemPrompt() gọi để
// mô tả các tool cho LLM. Chỉ liệt kê tool đang được phép dùng (isAllowed). ===
std::string ToolRegistry::generateToolsPrompt() const {
    std::string prompt = "Danh sach cac tool ban co the su dung:\n";
    for (const auto& [name, tool] : tools_) {
        if (!isAllowed(name)) continue; // Tool bị cấm thì không cho LLM thấy
        prompt += "- " + name + ": " + tool->getDescription() + "\n";
    }
    return prompt;
}