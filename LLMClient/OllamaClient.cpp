#include "json.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <expected>
#include "base64.h"
#include <curl/curl.h>
using json = nlohmann::json;
using namespace std;
enum class OllamaError{
    timeout,
    connectiondeny,
    malformjson,
    unknownednetwork
};
class OllamaClient{
    private:
    string gistID;
    string gistPAT;
    string modelName;
    float temperature;
    int max_tokens;
    public:
    OllamaClient(string model, string GISTID, string GISTPAT){
        modelName = model;
        gistID = GISTID;
        gistPAT = GISTPAT;
    }
    string encodeImageToBase64(const string& filePath) { //encode image to binary
        ifstream file(filePath, ios::binary);
        if (!file.is_open()) {
            cerr << "Error: Cannot open image file at " << filePath << endl;
            return ""; 
        }

        ostringstream ostrm;
        ostrm << file.rdbuf();
        string binaryData = ostrm.str();

        // Calling the cpp-base64 library function here
        return base64_encode(reinterpret_cast<const unsigned char*>(binaryData.data()), binaryData.size());
    }
    string buildPayload(const string& promptText, const string& base64Image = "") { // to build a json file
        json payload = {
            {"model", modelName},
            {"stream", false},
            {"options", {
                {"temperature", temperature},
                {"num_predict", max_tokens} // Ollama dùng num_predict thay vì max_tokens
            }}
        };

        // Tạo object message
        json message = {
            {"role", "user"},
            {"content", promptText}
        };

        // Nếu có chuỗi Base64 ảnh được truyền vào, nhét nó vào mảng "images"
        if (!base64Image.empty()) {
            message["images"] = json::array({base64Image});
        }

        // Nhét message vào mảng messages của payload tổng
        payload["messages"] = json::array({message});

        // Trả về chuỗi JSON hoàn chỉnh để curl bắn đi
        return payload.dump();
    }

    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // -------------------------------------------------------------------
    // CORE NETWORK FUNCTION: Handles both GET (for Gist) and POST (for Ollama)
    // -------------------------------------------------------------------
    std::expected<string, OllamaError> executeCurl(const string& url, const string& method = "GET", const string& payload = "") { //get link from GIST using "GET" as parameters, send json format to model using "POST"
        CURL* curl;
        CURLcode res;
        string readBuffer;

        curl = curl_easy_init();
        if (!curl) {
            return std::unexpected(OllamaError::unknownednetwork);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        // Critical for preventing infinite hangs
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); 
        
        // Bypass SSL checks (sometimes Pinggy domains cause SSL certificate errors)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: C++-Agent-Client");

        if (method == "POST") {
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        }

        // If you are fetching a private Gist, you need the Authorization header
        if (method == "GET" && !gistPAT.empty()) {
            string authHeader = "Authorization: Bearer " + gistPAT;
            headers = curl_slist_append(headers, authHeader.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Execute the request
        res = curl_easy_perform(curl);
        
        // Cleanup immediately
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        // Handle libcurl Errors using std::expected
        if (res != CURLE_OK) {
            if (res == CURLE_OPERATION_TIMEDOUT) {
                return std::unexpected(OllamaError::timeout);
            } else if (res == CURLE_COULDNT_CONNECT || res == CURLE_COULDNT_RESOLVE_HOST) {
                return std::unexpected(OllamaError::connectiondeny);
            }
            return std::unexpected(OllamaError::unknownednetwork);
        }

        return readBuffer;
    }
    std::expected<string, OllamaError> fetchActivePinggyUrl() { // return the ultimate link that we can use to contact with model
        string gistApiUrl = "https://api.github.com/gists/" + gistID;
        
        auto getResponse = executeCurl(gistApiUrl, "GET");
        
        if (!getResponse.has_value()) {
            return std::unexpected(getResponse.error()); // Propagate network error
        }

        try {
            auto gistJson = json::parse(*getResponse);
            
            // Extract the content from the specific file in your Gist
            // Make sure "ollama_url.txt" matches the filename in your Python script!
            string pinggyUrl = gistJson["files"]["ollama_url.txt"]["content"];
            
            // Clean up any trailing newlines or spaces from the URL
            pinggyUrl.erase(pinggyUrl.find_last_not_of(" \n\r\t") + 1);
            
            return pinggyUrl + "/api/chat"; // Append the specific Ollama endpoint
            
        } catch (json::parse_error& e) {
            return std::unexpected(OllamaError::malformjson);
        }
    }
    std::expected<string, OllamaError> processRequest(const string& jsonPayload) {
        // Step 1: Discover the active URL dynamically
        auto endpointResult = fetchActivePinggyUrl();
        if (!endpointResult.has_value()) {
            return std::unexpected(endpointResult.error());
        }
        string activeUrl = *endpointResult;

        // Step 2: Send the payload to that URL
        auto rawResponse = executeCurl(activeUrl, "POST", jsonPayload);
        
        if (!rawResponse.has_value()) {
            return std::unexpected(rawResponse.error());
        }

        // Step 3: Parse the result
        try {
            auto resJson = json::parse(*rawResponse);
            if (resJson.contains("message") && resJson["message"].contains("content")) {
                return resJson["message"]["content"].get<string>();
            } else {
                return std::unexpected(OllamaError::malformjson);
            }
        } catch (json::parse_error& e) {
            return std::unexpected(OllamaError::malformjson);
        }
        
    }
    std::expected<string, OllamaError> sendPrompt(const string& text) {
        // Build JSON payload không có ảnh
        string payload = buildPayload(text, ""); 
        
        // Đẩy payload vào lõi xử lý
        return processRequest(payload); 
    }
};
int main(){
    OllamaClient myAI("qwen2.5vl:3b","b54e95a50b8b9f0e7743a22593bc0503","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");
    auto result = myAI.sendPrompt("Mày đủ trình không!??");
    if (result.has_value()) {
        cout << "\n=== AI TRA LOI THANG CONG ===" << endl;
        cout << *result << endl; 
    } else {
        cout << "\n=== TOANG! CO LOI XAY RA ===" << endl;
        
        // In ra lỗi cụ thể
        if (result.error() == OllamaError::timeout) cout << "Loi: Timeout (Qua 60 giay)!" << endl;
        else if (result.error() == OllamaError::connectiondeny) cout << "Loi: Khong ket noi duoc (Check lai Pinggy)!" << endl;
        else if (result.error() == OllamaError::malformjson) cout << "Loi: JSON bi rach/sai dinh dang!" << endl;
        else cout << "Loi: Mang bat on!" << endl;
    }

}