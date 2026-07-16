#include "translate_tool.h"
#include "../LLMClient/json.hpp"
#include <curl/curl.h>
#include <iostream>

using json = nlohmann::json;

// ─── Helpers hạ tầng (giống style của WebSearchTool) ──────────────────────────

size_t TranslateTool::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string TranslateTool::urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;
    char* out = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
    std::string result = out ? out : value;
    curl_free(out);
    curl_easy_cleanup(curl);
    return result;
}

std::string TranslateTool::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> TranslateTool::splitPipe(const std::string& s) {
    std::vector<std::string> parts;
    size_t pos = 0;
    while (true) {
        size_t bar = s.find('|', pos);
        if (bar == std::string::npos) {
            parts.push_back(trim(s.substr(pos)));
            break;
        }
        parts.push_back(trim(s.substr(pos, bar - pos)));
        pos = bar + 1;
    }
    return parts;
}

// ─── Gọi MyMemory Translation API (https://mymemory.translated.net/) ─────────
// API miễn phí, không cần API key. Endpoint: GET /get?q=<text>&langpair=<src>|<dst>

std::string TranslateTool::callTranslateApi(const std::string& text,
                                             const std::string& sourceLang,
                                             const std::string& targetLang,
                                             std::string& errorOut) {
    std::string url = "https://api.mymemory.translated.net/get?q=" + urlEncode(text)
                     + "&langpair=" + urlEncode(sourceLang) + "|" + urlEncode(targetLang);

    CURL* curl = curl_easy_init();
    if (!curl) { errorOut = "khong the khoi tao cURL"; return ""; }

    std::string buffer;
    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,     "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) { errorOut = "khong ket noi duoc dich vu dich thuat (MyMemory)"; return ""; }

    try {
        auto j = json::parse(buffer);

        if (j.contains("responseStatus")) {
            int status = 0;
            if (j["responseStatus"].is_number()) status = j["responseStatus"].get<int>();
            else if (j["responseStatus"].is_string()) status = std::atoi(j["responseStatus"].get<std::string>().c_str());
            if (status != 0 && status != 200) {
                errorOut = "MyMemory tra ve loi (status " + std::to_string(status) + ")";
                return "";
            }
        }

        if (j.contains("responseData") && j["responseData"].contains("translatedText")) {
            std::string translated = j["responseData"]["translatedText"].get<std::string>();
            if (translated.empty()) { errorOut = "MyMemory tra ve ban dich rong"; return ""; }
            return translated;
        }

        errorOut = "Khong tim thay truong translatedText trong phan hoi.";
        return "";
    } catch (const json::parse_error&) {
        errorOut = "Loi parse JSON tu dich vu dich thuat.";
        return "";
    }
}

// ─── execute() ────────────────────────────────────────────────────────────────
// arguments: "<text>" | "<text> | <target>" | "<text> | <source> | <target>"

std::string TranslateTool::execute(const std::string& arguments) {
    std::string raw = trim(arguments);
    // Lột dấu nháy kép/nháy đơn bọc ngoài (phòng hờ LLM tự thêm)
    if (raw.size() >= 2 &&
        ((raw.front() == '"' && raw.back() == '"') ||
         (raw.front() == '\'' && raw.back() == '\''))) {
        raw = raw.substr(1, raw.size() - 2);
    }

    if (raw.empty()) return "Loi: van ban can dich dang rong.";

    std::vector<std::string> parts = splitPipe(raw);

    std::string text        = parts.size() >= 1 ? parts[0] : "";
    std::string sourceLang  = "vi";   // mac dinh: nguon la Tieng Viet
    std::string targetLang  = "en";   // mac dinh: dich sang Tieng Anh (phuc vu web_search)

    if (parts.size() == 2) {
        // "<text> | <target_lang>"
        targetLang = parts[1].empty() ? targetLang : parts[1];
    } else if (parts.size() >= 3) {
        // "<text> | <source_lang> | <target_lang>"
        sourceLang = parts[1].empty() ? sourceLang : parts[1];
        targetLang = parts[2].empty() ? targetLang : parts[2];
    }

    if (text.empty()) return "Loi: van ban can dich dang rong.";

    std::cout << "[TranslateTool] Dich (" << sourceLang << " -> " << targetLang
               << "): " << text << "\n";

    std::string errorMsg;
    std::string translated = callTranslateApi(text, sourceLang, targetLang, errorMsg);

    if (translated.empty()) {
        return "Loi dich thuat: " + (errorMsg.empty() ? std::string("khong ro nguyen nhan") : errorMsg) +
               ". Ban co the thu lai voi van ban ngan gon hon, hoac tu dich thu cong roi goi web_search.";
    }

    return "Ban dich (" + sourceLang + "->" + targetLang + "): " + translated;
}
