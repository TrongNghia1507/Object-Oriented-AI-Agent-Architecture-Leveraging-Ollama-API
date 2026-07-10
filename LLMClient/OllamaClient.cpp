#include "OllamaClient.h"
#include <vector>
#include "json.hpp"
#include "base64.h"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>

using json = nlohmann::json;
using namespace std;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Constructor đầy đủ (private) - không khai default argument lại ở đây,
// vì đó là nguyên nhân gây lỗi biên dịch "redefinition of default argument"
// (default argument chỉ được khai DUY NHẤT một lần, ở khai báo trong .h).
OllamaClient::OllamaClient(string model, string baseUrl, string gistId, string gistPAT)
    : modelName_(std::move(model)),
      baseUrl_(std::move(baseUrl)),
      gistId_(std::move(gistId)),
      gistPAT_(std::move(gistPAT)) {}

OllamaClient::OllamaClient(string model, string baseUrl)
    : OllamaClient(std::move(model), std::move(baseUrl), "", "") {}

OllamaClient OllamaClient::withGistDiscovery(string model, string gistId, string gistPAT) {
    return OllamaClient(std::move(model), "", std::move(gistId), std::move(gistPAT));
}

string OllamaClient::encodeImageToBase64(const string& filePath) {
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Cannot open image file at " << filePath << endl;
        return "";
    }
    ostringstream ostrm;
    ostrm << file.rdbuf();
    string binaryData = ostrm.str();
    return base64_encode(reinterpret_cast<const unsigned char*>(binaryData.data()), binaryData.size());
}

string OllamaClient::buildPayload(const string& promptText, const string& base64Image) {
    json payload = {
        {"model", modelName_},
        {"stream", false},
        {"options", {
            {"temperature", temperature_},
            {"num_predict", maxTokens_}, // Ollama dùng num_predict thay vì max_tokens
            {"stop", {"Observation:", "Final Answer:"}}
        }}
    };

    json message = {
        {"role", "user"},
        {"content", promptText}
    };

    if (!base64Image.empty()) {
        message["images"] = json::array({base64Image});
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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    // Bypass SSL checks (đôi khi domain Pinggy gây lỗi SSL certificate)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: C++-Agent-Client");

    if (method == "POST") {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    }

    if (method == "GET" && !gistPAT_.empty()) {
        string authHeader = "Authorization: Bearer " + gistPAT_;
        headers = curl_slist_append(headers, authHeader.c_str());
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

    return readBuffer;
}

// Trả về endpoint /api/chat đầy đủ, theo đúng chế độ đã chọn khi khởi tạo.
// std::expected<string, LLMError> OllamaClient::resolveEndpoint() {
//     if (gistId_.empty()) {
//         // Chế độ 1: baseUrl cố định.
//         if (baseUrl_.empty()) {
//             return std::unexpected(LLMError::ConnectionDenied);
//         }
//         return baseUrl_ + "/api/chat";
//     }

//     // Chế độ 2: dò URL động qua Gist.
//     string gistApiUrl = "https://api.github.com/gists/" + gistId_;
//     auto getResponse = executeCurl(gistApiUrl, "GET", "");
//     if (!getResponse.has_value()) {
//         return std::unexpected(getResponse.error());
//     }

//     try {
//         auto gistJson = json::parse(*getResponse);
//         // Tên file trong Gist phải khớp với script ghi URL (vd. ollama_url.txt)
//         string pinggyUrl = gistJson["files"]["ollama_url.txt"]["content"];
//         pinggyUrl.erase(pinggyUrl.find_last_not_of(" \n\r\t") + 1);
//         return pinggyUrl + "/api/chat";
//     } catch (const json::parse_error&) {
//         return std::unexpected(LLMError::MalformedJson);
//     } catch (const json::out_of_range&) {
//         // Gist tồn tại nhưng không có file "ollama_url.txt" như mong đợi
//         return std::unexpected(LLMError::MalformedJson);
//     }
// }

// std::expected<string, LLMError> OllamaClient::processRequest(const string& jsonPayload) {
//     auto endpointResult = resolveEndpoint();
//     if (!endpointResult.has_value()) {
//         return std::unexpected(endpointResult.error());
//     }
//     string activeUrl = *endpointResult;

//     auto rawResponse = executeCurl(activeUrl, "POST", jsonPayload);
//     if (!rawResponse.has_value()) {
//         return std::unexpected(rawResponse.error());
//     }

//     try {
//         auto resJson = json::parse(*rawResponse);
//         if (resJson.contains("message") && resJson["message"].contains("content")) {
//             return resJson["message"]["content"].get<string>();
//         }
//         return std::unexpected(LLMError::MalformedJson);
//     } catch (const json::parse_error&) {
//         return std::unexpected(LLMError::MalformedJson);
//     }
// }
std::expected<string, LLMError> OllamaClient::resolveEndpoint() {
    if (gistId_.empty()) {
        if (baseUrl_.empty()) {
            return std::unexpected(LLMError::ConnectionDenied);
        }
        return baseUrl_ + "/api/chat";
    }

    string gistApiUrl = "https://api.github.com/gists/" + gistId_;
    auto getResponse = executeCurl(gistApiUrl, "GET", "");
    if (!getResponse.has_value()) {
        return std::unexpected(getResponse.error());
    }

    try {
        auto gistJson = json::parse(*getResponse);
        
        // --- MẮT THẦN SOI GITHUB ---
        if (!gistJson.contains("files")) {
            std::cout << "\033[31m[CRITICAL] Github Gist tu choi truy cap! Gist ID hoac Token cua bro co van de. Github chui:\033[0m\n" 
                      << gistJson.dump(2) << "\n";
            return std::unexpected(LLMError::MalformedJson);
        }
        
        if (!gistJson["files"].contains("ollama_url.txt") || !gistJson["files"]["ollama_url.txt"].contains("content")) {
            std::cout << "\033[31m[CRITICAL] Trong Gist khong co file 'ollama_url.txt' hoac file bi rong!\033[0m\n";
            return std::unexpected(LLMError::MalformedJson);
        }
        // -----------------------------

        string pinggyUrl = gistJson["files"]["ollama_url.txt"]["content"].get<string>();
        pinggyUrl.erase(pinggyUrl.find_last_not_of(" \n\r\t") + 1);
        return pinggyUrl + "/api/chat";
        
    } catch (const std::exception& e) {
        std::cout << "\033[31m[CRITICAL] Loi Parse Gist JSON: \033[0m" << e.what() << "\n";
        return std::unexpected(LLMError::MalformedJson);
    }
}
std::expected<string, LLMError> OllamaClient::processRequest(const string& jsonPayload) {
    auto endpointResult = resolveEndpoint();
    if (!endpointResult.has_value()) {
        return std::unexpected(endpointResult.error());
    }
    string activeUrl = *endpointResult;

    // [MẮT THẦN 1] In ra URL để xem Pinggy có đúng không
    std::cout << "\n\033[33m[DEBUG-LLM] Dang ket noi toi: \033[0m" << activeUrl << "\n";

    auto rawResponse = executeCurl(activeUrl, "POST", jsonPayload);
    if (!rawResponse.has_value()) {
        std::cout << "\033[31m[DEBUG-LLM] Loi mang/Timeout roi!\033[0m\n";
        return std::unexpected(rawResponse.error());
    }

    // [MẮT THẦN 2] In sạch sẽ toàn bộ những gì Server trả về trước khi Parse
    std::cout << "\033[36m[DEBUG-LLM] Server tra ve nguyen goc:\n\033[0m" 
              << *rawResponse << "\n\n";

    try {
        auto resJson = json::parse(*rawResponse);

        // === BỔ SUNG: đọc token usage thật từ response Ollama ===
        // Ollama's /api/chat trả kèm "prompt_eval_count" (số token của input)
        // và "eval_count" (số token model sinh ra) ở object gốc, không nằm
        // trong "message". Trước đây các trường này bị bỏ qua hoàn toàn nên
        // Step::tokensUsed luôn = 0. Dùng .value(key, 0) để an toàn với các
        // bản Ollama cũ/model không trả về các trường này.
        int promptTokens = resJson.value("prompt_eval_count", 0);
        int completionTokens = resJson.value("eval_count", 0);
        lastTokenUsage_ = promptTokens + completionTokens;

        if (resJson.contains("message") && resJson["message"].contains("content")) {
            return resJson["message"]["content"].get<string>();
        }

        if (resJson.contains("message") && resJson["message"].contains("content")) {
            // [KHIÊN BẢO VỆ] Kiem tra chac chan no la Chuoi (String) roi moi lay ra!
            if (resJson["message"]["content"].is_string()) {
                return resJson["message"]["content"].get<string>();
            } else {
                std::cout << "\033[31m[DEBUG-LLM] Truong 'content' bi NULL hoac khong phai chuoi!\033[0m\n";
                return std::unexpected(LLMError::MalformedJson);
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
        {"options", {
            {"temperature", temperature_},
            {"num_predict", maxTokens_}
        }}
    };
    json arr = json::array();
    for (const auto& m : messages) {
        arr.push_back({{"role", m.role}, {"content", m.content}});
    }
    payload["messages"] = arr;
    
    return processRequest(payload.dump());
}

std::expected<string, LLMError> OllamaClient::sendPromptWithImage(const string& text, const string& imagePath) {
    string base64Image = encodeImageToBase64(imagePath);
    if (base64Image.empty()) {
        return std::unexpected(LLMError::UnknownNetworkError);
    }
    string payload = buildPayload(text, base64Image);
    return processRequest(payload);
}
