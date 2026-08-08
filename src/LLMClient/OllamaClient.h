#pragma once
#include "LLMClient.h"
#include <vector>
#include <string>

class OllamaClient : public LLMClient {
private:
    std::string modelName_;
    std::string baseUrl_;
    float temperature_ = 0.7f;   // có giá trị mặc định hợp lý, KHÔNG còn để rác chưa khởi tạo
    int maxTokens_ = 2048;

    // === BỔ SUNG: lưu tổng token (prompt_eval_count + eval_count) mà Ollama
    // trả về ở lần gọi processRequest() gần nhất, để AgentLoop đọc lại qua
    // getLastTokenUsage() sau mỗi bước và ghi đúng vào Step::tokensUsed.
    int lastTokenUsage_ = 0;

    std::string buildPayload(const std::string& promptText, const std::string& base64Image);
    std::expected<std::string, LLMError> executeCurl(const std::string& url,
                                                       const std::string& method,
                                                       const std::string& payload);
    std::expected<std::string, LLMError> resolveEndpoint();
    std::expected<std::string, LLMError> processRequest(const std::string& jsonPayload);
public:
    OllamaClient(std::string model, std::string baseUrl);

    std::expected<std::string, LLMError> sendPrompt(const std::string& text) override;
    std::expected<std::string, LLMError> sendMessages(const std::vector<Message>& messages) override;
    std::expected<std::string, LLMError> sendPromptWithImage(
        const std::string& text, const std::string& imagePath) override;

    void setTemperature(float temperature) override { temperature_ = temperature; }
    void setMaxTokens(int maxTokens) override { maxTokens_ = maxTokens; }
    std::string getModelName() const override { return modelName_; }
    int getLastTokenUsage() const override {return lastTokenUsage_;}
};
