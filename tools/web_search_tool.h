#pragma once
#include "tool.h"
#include <string>

class WebSearchTool : public Tool {
public:
    WebSearchTool() = default;
    ~WebSearchTool() override = default;

    std::string getName() const override { return "web_search"; }
    std::string getDescription() const override {
        return "Tim kiem tren SearXNG (Tong hop ket qua tu Google, Bing... — "
               "phu hop de tim tin tuc, bai viet chi tiet, su kien thoi su). "
               "Tham so: tu khoa Tieng Anh, vi du: 'Vietnam inflation rate' hay 'Albert Einstein'.";
    }
    // Nhận từ khóa tìm kiếm dạng chuỗi thuần.
    std::string execute(const std::string& arguments) override;

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::string urlEncode(const std::string& value);
};