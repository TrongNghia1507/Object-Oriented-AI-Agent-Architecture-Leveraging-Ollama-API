#include "web_search_tool.h"
#include "../LLMClient/json.hpp"
#include <curl/curl.h>
#include <iostream>

using json = nlohmann::json;

size_t WebSearchTool::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string WebSearchTool::urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;
    char* out = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
    std::string result = out ? out : value;
    curl_free(out);
    curl_easy_cleanup(curl);
    return result;
}

// execute(): nhận từ khóa thuần (AgentLoop đã bóc tách trước khi gọi).
// Không cần tự parse JSON — đây là lý do bỏ 4 code path cũ.
std::string WebSearchTool::execute(const std::string& arguments) {
    // Trim khoảng trắng
    std::string query = arguments;
    while (!query.empty() && (query.front() == ' ' || query.front() == '\t')) query.erase(0, 1);
    while (!query.empty() && (query.back()  == ' ' || query.back()  == '\t')) query.pop_back();

    if (query.empty()) return "Loi: tu khoa tim kiem rong.";

    std::cout << "[WebSearchTool] Tim kiem: " << query << "\n";

    std::string url = "https://api.duckduckgo.com/?q=" + urlEncode(query)
                      + "&format=json&no_html=1&skip_disambig=1";

    CURL* curl = curl_easy_init();
    if (!curl) return "Loi: khong the khoi tao cURL.";

    std::string buffer;
    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,     "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "Loi mang: khong ket noi duoc DuckDuckGo.";

    try {
        auto j = json::parse(buffer);
        std::string out = "KET QUA cho '" + query + "':\n";
        bool found = false;

        if (j.contains("AbstractText") && j["AbstractText"].is_string()) {
            std::string text = j["AbstractText"].get<std::string>();
            if (!text.empty()) { out += "[Tom tat]: " + text + "\n"; found = true; }
        }
        if (j.contains("RelatedTopics") && j["RelatedTopics"].is_array()) {
            int count = 0;
            for (const auto& t : j["RelatedTopics"]) {
                if (t.contains("Text") && t["Text"].is_string()) {
                    out += "- " + t["Text"].get<std::string>() + "\n";
                    found = true;
                    if (++count >= 3) break;
                }
            }
        }
        if (!found) {
            return "DuckDuckGo khong tra ve ket qua cu the. "
                   "Luu y: API nay chi hieu qua voi ten rieng / dinh nghia lon (Wikipedia-level). "
                   "Hay thu tu khoa Tieng Anh ngan gon hon.";
        }
        return out;

    } catch (const json::parse_error&) {
        return "Loi parse JSON tu DuckDuckGo.";
    }
}
