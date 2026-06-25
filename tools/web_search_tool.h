#pragma once
#include "tool.h"
#include <string>

class WebSearchTool : public Tool {
public:
    WebSearchTool() = default;
    ~WebSearchTool() override = default;

    std::string getName() const override { return "web_search"; }
    std::string getDescription() const override {
        return "Tim kiem tren DuckDuckGo (Instant Answer API — phu hop voi ten rieng, "
               "dinh nghia, su kien lon). Tham so: tu khoa Tieng Anh ngan gon, "
               "vi du: 'Eiffel Tower height' hay 'Albert Einstein'.";
    }
    // Nhận từ khóa tìm kiếm dạng chuỗi thuần.
    std::string execute(const std::string& arguments) override;

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::string urlEncode(const std::string& value);
};
