#ifndef WEB_SEARCH_TOOL_H
#define WEB_SEARCH_TOOL_H

#include "tool.h" // Nhúng interface gốc của bạn
#include <string>

class WebSearchTool : public Tool {
public:
    WebSearchTool();
    ~WebSearchTool() override;

    // 1. SỬA LẠI: Tham số truyền vào phải là `const json&` để khớp 100% với class Tool
    std::string execute(const json& payload) override;

    std::string getName() const override { return "web_search"; }

    // 2. BỔ SUNG: Bắt buộc phải thêm hàm getDescription() do class Tool yêu cầu
    std::string getDescription() const override { 
        return "Công cụ tra cứu thông tin trên Internet. Tham số là từ khóa tìm kiếm."; 
    }

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::string urlEncode(const std::string& value);
};

#endif // WEB_SEARCH_TOOL_H