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

std::string WebSearchTool::execute(const std::string& arguments) {
    std::string query = arguments;
    while (!query.empty() && (query.front() == ' ' || query.front() == '\t')) query.erase(0, 1);
    while (!query.empty() && (query.back()  == ' ' || query.back()  == '\t')) query.pop_back();

    if (query.empty()) return "Loi: tu khoa tim kiem rong.";

    std::cout << "[WebSearchTool] Tim kiem SearXNG: " << query << "\n";

    // DANH SÁCH MÁY CHỦ DỰ PHÒNG (Tự động chuyển đổi nếu máy chủ trước bị lỗi)
    std::vector<std::string> searxng_endpoints = {
        "http://127.0.0.1:8080/search"
    };

    CURL* curl = curl_easy_init();
    if (!curl) return "Loi: khong the khoi tao cURL.";

    std::string buffer;
    bool success = false;
    std::string last_error = "";

    // Duyệt qua từng endpoint, nếu lỗi sẽ tự động thử endpoint tiếp theo
    for (const auto& base_url : searxng_endpoints) {
        std::string url = base_url + "?q=" + urlEncode(query) + "&format=json";
        
        buffer.clear();
        curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buffer);
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json, text/plain, */*");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Chờ tối đa 10s cho mỗi server
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); 
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);

        // Nếu gọi curl thành công và có data
        if (res == CURLE_OK && buffer.length() > 0) {
            try {
                auto j = json::parse(buffer);
                if (j.contains("results") && j["results"].is_array()) {
                    success = true;
                    break; // Thành công thì thoát vòng lặp, không cần thử server khác
                } else if (j.contains("error")) {
                    last_error = "API tra ve loi: " + j["error"].get<std::string>();
                }
            } catch (const json::parse_error&) {
                last_error = "Loi parse JSON tu " + base_url;
            }
        } else {
            last_error = "Khong the ket noi toi " + base_url;
        }
    }

    curl_easy_cleanup(curl);

    // XỬ LÝ KẾT QUẢ NẾU THÀNH CÔNG
    if (success) {
        try {
            auto j = json::parse(buffer);
            std::string out = "KET QUA TIM KIEM CHO '" + query + "':\n\n";
            int count = 0;
            
            for (const auto& item : j["results"]) {
                std::string title = item.value("title", "Khong co tieu de");
                std::string link = item.value("url", "");
                std::string snippet = item.value("content", "");
                if (snippet.empty()) snippet = item.value("snippet", "");

                out += "[" + std::to_string(count + 1) + "] " + title + "\n";
                out += "Link: " + link + "\n";
                out += "Snippet: " + snippet + "\n\n";
                
                if (++count >= 5) break;
            }
            if (count == 0) return "SearXNG khong tim thay ket qua nao.";
            return out;
        } catch (...) {
            return "Loi trich xuat data JSON.";
        }
    }

    // NẾU TẤT CẢ SERVER ĐỀU THẤT BẠI
    return "Loi: Khong the tim kiem. Tat ca may chu SearXNG deu that bai. Chi tiet loi cuoi: " + last_error;
}