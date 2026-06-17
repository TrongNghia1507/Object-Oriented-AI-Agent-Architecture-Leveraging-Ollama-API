#include "tool_registry.h"
#include <iostream>

void ToolRegistry::registerTool(std::unique_ptr<Tool> tool) {
    if (tool == nullptr) return;
    
    std::string name = tool->getName();
    // Nếu tool chưa tồn tại trong map, tiến hành nạp vào kho
    if (registry_.find(name) == registry_.end()) {
        registry_[name] = std::move(tool); // Chuyển quyền sở hữu con trỏ vào map
    } else {
        std::cout << "[Warning] Tool voi ten '" << name << "' da duoc dang ky truoc do." << std::endl;
    }
}

Tool* ToolRegistry::getTool(const std::string& name) const {
    auto it = registry_.find(name);
    if (it != registry_.end()) {
        return it->second.get(); // Trả về con trỏ thô (raw pointer) để sử dụng, không chuyển quyền sở hữu
    }
    return nullptr; // Khong tim thay tool tương ứng
}

std::vector<std::string> ToolRegistry::getAllToolNames() const {
    std::vector<std::string> names;
    for (const auto& pair : registry_) {
        names.push_back(pair.first);
    }
    return names;
}

std::string ToolRegistry::generateToolsPrompt() const {
    std::string prompt = "He thong cua ban co cac cong cu (Tools) sau day de ho tro giai quyet bieu thuc:\n";
    for (const auto& pair : registry_) {
        prompt += "- Tên công cụ: " + pair.second->getName() + "\n";
        prompt += "  Mo ta: " + pair.second->getDescription() + "\n\n";
    }
    return prompt;
}