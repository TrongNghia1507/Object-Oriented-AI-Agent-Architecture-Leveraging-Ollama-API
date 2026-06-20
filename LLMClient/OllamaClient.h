#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <expected>
enum class LLMError{
    timeout,
    connectiondeny,
    malformjson,
    unknownednetwork
};
class LLMClient{
    protected:
    std::string modelName;
    float temperature;
    int max_tokens;
    public:
    LLMClient(std::string model){
        this->modelName = model;
    }
    virtual std::expected<std::string, LLMError> sendPrompt(const std::string& text) = 0;
    virtual std::expected<std::string, LLMError> sendPrompt(const std::string& text,const std::string& imagePath) = 0;

    virtual ~LLMClient() = default;
};
class OllamaClient : public LLMClient{
    private:
    std::string gistID;
    std::string gistPAT;
    public:
    OllamaClient(std::string model, std::string GISTID, std::string GISTPAT);
    std::string encodeImageToBase64(const std::string& filePath);
    std::string buildPayload(const std::string& promptText, const std::string& base64Image = "");
    std::expected<std::string, LLMError> executeCurl(const std::string& url, const std::string& method = "GET", const std::string& payload = "");
    std::expected<std::string,LLMError> fetchActivePinggyUrl();
    std::expected<std::string, LLMError> processRequest(const std::string& jsonPayload) ;
    std::expected<std::string, LLMError> sendPrompt(const std::string& text) override ;

};