#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <expected>
enum class OllamaError{
    timeout,
    connectiondeny,
    malformjson,
    unknownednetwork
};
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
class OllamaClient{
    private:
    std::string gistID;
    std::string gistPAT;
    std::string modelName;
    float temperature;
    int max_tokens;
    public:
    OllamaClient(std::string model, std::string GISTID, std::string GISTPAT);
    std::string encodeImageToBase64(const std::string& filePath);
    std::string buildPayload(const std::string& promptText, const std::string& base64Image = "");
    std::expected<std::string, OllamaError> executeCurl(const std::string& url, const std::string& method = "GET", const std::string& payload = "");
    std::expected<std::string, OllamaError> fetchActivePinggyUrl();
    std::expected<std::string, OllamaError> processRequest(const std::string& jsonPayload) ;
    std::expected<std::string, OllamaError> sendPrompt(const std::string& text);

};