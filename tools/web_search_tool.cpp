#include "web_search_tool.h"
#include "../LLMClient/json.hpp" // Đảm bảo đường dẫn này đúng
#include <curl/curl.h>
#include <iostream>

using json = nlohmann::json;

WebSearchTool::WebSearchTool() {}

WebSearchTool::~WebSearchTool() {}

// Hàm Callback dùng để hứng dữ liệu trả về từ cURL
size_t WebSearchTool::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

// Hàm mã hóa URL 
std::string WebSearchTool::urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (curl) {
        char* output = curl_easy_escape(curl, value.c_str(), value.length());
        if (output) {
            std::string encoded(output);
            curl_free(output);
            curl_easy_cleanup(curl);
            return encoded;
        }
        curl_easy_cleanup(curl);
    }
    return value;
}

// Hàm cốt lõi: Thực thi logic lấy dữ liệu bằng DuckDuckGo
std::string WebSearchTool::execute(const json& payload) {
    std::string args = "";

    // 1. Bóc tách chuỗi tìm kiếm từ payload JSON một cách an toàn
    try {
        if (payload.is_string()) {
            args = payload.get<std::string>();
        } else if (payload.contains("query")) {
            args = payload["query"].get<std::string>();
        } else if (payload.contains("arguments")) {
            args = payload["arguments"].get<std::string>();
        } else {
            args = payload.dump();
            if (args.length() >= 2 && args.front() == '"' && args.back() == '"') {
                args = args.substr(1, args.length() - 2);
            }
        }
    } catch (...) {
        return "Lỗi: Không thể đọc tham số (Arguments) JSON do AI cung cấp.";
    }

    if (args.empty()) return "Lỗi: Vui lòng cung cấp từ khóa tìm kiếm.";

    std::cout << "[WebSearchTool] Đang tìm kiếm DuckDuckGo cho: " << args << "...\n";

    // 2. Chuẩn bị gọi API DuckDuckGo
    // Tham số no_html=1 để loại bỏ thẻ HTML, format=json để lấy JSON
    std::string encodedQuery = urlEncode(args);
    std::string url = "https://api.duckduckgo.com/?q=" + encodedQuery + "&format=json&no_html=1&skip_disambig=1";

    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (!curl) return "Lỗi: Không thể khởi tạo cURL.";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    // DuckDuckGo đôi khi chặn các request không có User-Agent chuẩn
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"); 
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    // Thực thi lấy dữ liệu
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return "Lỗi mạng: Không thể kết nối tới DuckDuckGo.";
    }

    // 3. Parse kết quả JSON trả về từ DuckDuckGo
    try {
        auto jsonResponse = json::parse(readBuffer);
        std::string finalResult = "KẾT QUẢ DUCKDUCKGO CHO TỪ KHÓA '" + args + "':\n";
        bool hasData = false;

        // Ưu tiên 1: Lấy câu trả lời tóm tắt trực tiếp (AbstractText)
        if (jsonResponse.contains("AbstractText") && jsonResponse["AbstractText"].is_string()) {
            std::string abstractText = jsonResponse["AbstractText"].get<std::string>();
            if (!abstractText.empty()) {
                finalResult += "[Tóm tắt nội dung]: " + abstractText + "\n";
                hasData = true;
            }
        }

        // Ưu tiên 2: Nếu không có tóm tắt, lấy các chủ đề liên quan (RelatedTopics)
        if (jsonResponse.contains("RelatedTopics") && jsonResponse["RelatedTopics"].is_array()) {
            auto topics = jsonResponse["RelatedTopics"];
            int count = 0;
            for (const auto& topic : topics) {
                // Chỉ lấy các topic có chứa văn bản (bỏ qua các topic phân nhánh đệ quy)
                if (topic.contains("Text") && topic["Text"].is_string()) {
                    std::string text = topic["Text"].get<std::string>();
                    finalResult += "- [Liên quan]: " + text + "\n";
                    hasData = true;
                    count++;
                    if (count >= 3) break; // Chỉ lấy tối đa 3 kết quả để không làm tràn context của AI
                }
            }
        }

        if (!hasData) {
            return "DuckDuckGo không trả về thông tin cụ thể nào cho từ khóa này. Hãy thử từ khóa khác.";
        }

        return finalResult;

    } catch (json::parse_error& e) {
        return "Lỗi parse JSON: Dữ liệu API trả về bị hỏng hoặc sai định dạng.";
    }
}