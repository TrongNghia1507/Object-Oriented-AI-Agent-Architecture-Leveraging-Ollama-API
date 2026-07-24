#include "OllamaClient.h"
#include <vector>
#include "json.hpp"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "../utils/SystemUtils.h"
using json = nlohmann::json;
using namespace std;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Constructor đầy đủ (private) - không khai default argument lại ở đây,
// vì đó là nguyên nhân gây lỗi biên dịch "redefinition of default argument"
// (default argument chỉ được khai DUY NHẤT một lần, ở khai báo trong .h).
OllamaClient::OllamaClient(string model, string baseUrl)
    : modelName_(std::move(model)),
      baseUrl_(std::move(baseUrl)){}

string OllamaClient::buildPayload(const string& promptText, const string& base64Image) {
    json payload = {
        {"model", modelName_},
        {"stream", false},
        
        // CỦA OPENROUTER: Đưa toàn bộ config ra ngoài cùng, không bọc trong "options" nữa
        {"temperature", temperature_},
        {"max_tokens", maxTokens_}, 
        {"stop", {"Observation:", "Final Answer:"}}
    };

    json message = {
        {"role", "user"}
    };

    if (base64Image.empty()) {
        message["content"] = promptText;
    } else {
        message["content"] = json::array({
            {
                {"type", "text"},
                {"text", promptText}
            },
            {
                {"type", "image_url"},
                {"image_url", {
                    {"url", "data:image/jpeg;base64," + base64Image}
                }}
            }
        });
    }

    payload["messages"] = json::array({message});
    return payload.dump();
}
std::expected<string, LLMError> OllamaClient::executeCurl(const string& url, const string& method, const string& payload) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (!curl) {
        return std::unexpected(LLMError::UnknownNetworkError);
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // Có thể tăng lên 120L nếu model suy luận lâu

    // Bypass SSL checks (Bạn có thể đổi thành 1L nếu máy đã cài đủ chứng chỉ SSL, 
    // OpenRouter dùng HTTPS chuẩn nên rất an toàn)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: C++-Agent-Client");
    headers = curl_slist_append(headers, "HTTP-Referer: https://localhost");
    // Chỉ tập trung xử lý POST để gửi Prompt lên OpenRouter
    if (method == "POST") {
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Đọc API Key từ file .env (MỤC 2)
        const char* apiKeyEnv = std::getenv("OPENROUTER_API_KEY");
        if (apiKeyEnv) {
            std::string key(apiKeyEnv);
            
            key.erase(std::remove(key.begin(), key.end(), '\r'), key.end());
            key.erase(std::remove(key.begin(), key.end(), '\n'), key.end());
            key.erase(std::remove(key.begin(), key.end(), '\"'), key.end());
            key.erase(std::remove(key.begin(), key.end(), '\''), key.end());
            std::string authHeader = "Authorization: Bearer " + std::string(apiKeyEnv);
            headers = curl_slist_append(headers, authHeader.c_str());
        }

        // Định danh bắt buộc của OpenRouter (MỤC 2)

        // Ép Header và Payload vào cURL
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        if (res == CURLE_OPERATION_TIMEDOUT) {
            return std::unexpected(LLMError::Timeout);
        } else if (res == CURLE_COULDNT_CONNECT || res == CURLE_COULDNT_RESOLVE_HOST) {
            return std::unexpected(LLMError::ConnectionDenied);
        }
        return std::unexpected(LLMError::UnknownNetworkError);
    }

    // readBuffer lúc này sẽ CHỈ chứa chuỗi JSON kết quả trả về từ OpenRouter
    return readBuffer;
}
std::expected<string, LLMError> OllamaClient::resolveEndpoint() {
    if (baseUrl_.empty()) {// Trả về thẳng endpoint chuẩn nếu baseUrl_ chưa được gán
        return "https://openrouter.ai/api/v1/chat/completions";
    }

    // Nếu baseUrl_ là "https://openrouter.ai"
    // Nhớ đảm bảo chuỗi trả về đúng chuẩn API của họ
    if (baseUrl_ == "https://openrouter.ai") {
         return baseUrl_ + "/api/v1/chat/completions";
    }
    
    // Fallback cho trường hợp đã truyền full URL vào baseUrl_
    return baseUrl_;
}
std::expected<string, LLMError> OllamaClient::processRequest(const string& jsonPayload) {
    auto endpointResult = resolveEndpoint();
    if (!endpointResult.has_value()) {
        return std::unexpected(endpointResult.error());
    }
    string activeUrl = *endpointResult;

    // [MẮT THẦN 1] In ra URL (chữ Pinggy giờ không còn ý nghĩa, ta đang chĩa thẳng lên Cloud)
    std::cout << "\n\033[33m[DEBUG-LLM] Dang ket noi toi: \033[0m" << activeUrl << "\n";

    auto rawResponse = executeCurl(activeUrl, "POST", jsonPayload);
    if (!rawResponse.has_value()) {
        std::cout << "\033[31m[DEBUG-LLM] Loi mang/Timeout roi!\033[0m\n";
        return std::unexpected(rawResponse.error());
    }
    try {
        auto resJson = json::parse(*rawResponse);

        // === BỔ SUNG: Xử lý báo lỗi từ OpenRouter (Rất quan trọng) ===
        // Nếu API Key sai, hết hạn mức, hoặc tên model viết sai, OpenRouter sẽ trả về object "error"
        if (resJson.contains("error")) {
            std::cout << "\033[31m[DEBUG-LLM] OpenRouter tu choi request. Loi:\n\033[0m" 
                      << resJson["error"].dump(2) << "\n";
            return std::unexpected(LLMError::ConnectionDenied);
        }

        // === CẬP NHẬT 1: Đọc Token Usage theo chuẩn OpenRouter ===
        // OpenRouter gom toàn bộ thông tin token vào object con tên là "usage"
        if (resJson.contains("usage")) {
            int promptTokens = resJson["usage"].value("prompt_tokens", 0);
            int completionTokens = resJson["usage"].value("completion_tokens", 0);
            lastTokenUsage_ = promptTokens + completionTokens;
        } else {
            lastTokenUsage_ = 0;
        }

        // === CẬP NHẬT 2: Trích xuất nội dung tin nhắn ===
        // Đường dẫn mới là: resJson -> "choices" (Array) -> phần tử 0 -> "message" -> "content"
        if (resJson.contains("choices") && resJson["choices"].is_array() && !resJson["choices"].empty()) {
            auto& firstChoice = resJson["choices"][0];
            
            if (firstChoice.contains("message") && firstChoice["message"].contains("content")) {
                // [KHIÊN BẢO VỆ] Kiem tra chac chan no la Chuoi (String) roi moi lay ra!
                if (firstChoice["message"]["content"].is_string()) {
                    return firstChoice["message"]["content"].get<string>();
                } else {
                    std::cout << "\033[31m[DEBUG-LLM] Truong 'content' bi NULL hoac khong phai chuoi!\033[0m\n";
                    return std::unexpected(LLMError::MalformedJson);
                }
            }
        }
        
        return std::unexpected(LLMError::MalformedJson);
        
    } catch (const json::parse_error& e) {
        std::cout << "\033[31m[DEBUG-LLM] Khong the parse JSON. Loi: \033[0m" << e.what() << "\n";
        return std::unexpected(LLMError::MalformedJson);
    }
}
std::expected<string, LLMError> OllamaClient::sendPrompt(const string& text) {
    string payload = buildPayload(text, "");
    return processRequest(payload);
}
// Multi-turn: gửi toàn bộ conversation history lên /api/chat.
// AgentLoop xây vector<Message> qua nhiều bước rồi gọi method này.
std::expected<string, LLMError> OllamaClient::sendMessages(const vector<Message>& messages) {
    json payload = {
        {"model", modelName_},
        {"stream", false},
        {"temperature", temperature_},
        {"num_predict", maxTokens_}
    
    };
    json arr = json::array();
    for (const auto& m : messages) {
        json content_arr = json::array();

        content_arr.push_back({{"type", "text"}, {"text", m.content}});

        if (m.hasImage) {
            content_arr.push_back({
                {"type", "image_url"},
                {"image_url", {{"url", "data:image/png;base64," + m.image}}}
            });
        }

        arr.push_back({{"role", m.role}, {"content", content_arr}});
    }
    payload["messages"] = arr;
    
    return processRequest(payload.dump());
}

std::expected<string, LLMError> OllamaClient::sendPromptWithImage(const string& text, const string& imagePath) {
    string base64Image = SystemUtils::encodeImageToBase64(imagePath);
    if (base64Image.empty()) {
        return std::unexpected(LLMError::UnknownNetworkError);
    }
    string payload = buildPayload(text, base64Image);
    return processRequest(payload);
}
