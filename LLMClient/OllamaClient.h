#pragma once
#include "LLMClient.h"
#include <vector>
#include <string>

// OllamaClient: cài đặt cụ thể của LLMClient, nói chuyện với Ollama qua REST API /api/chat.
//
// Hỗ trợ 2 chế độ xác định base URL (đáp ứng yêu cầu "Cấu hình được: base URL"):
//   1) Truyền thẳng baseUrl, vd: OllamaClient("qwen2.5vl:3b", "http://localhost:11434")
//   2) Dò URL động qua GitHub Gist (dùng khi Ollama chạy ở máy khác, expose qua tunnel
//      Pinggy và script Python ghi URL hiện hành vào 1 Gist riêng):
//      OllamaClient::withGistDiscovery("qwen2.5vl:3b", gistId, gistPAT)
class OllamaClient : public LLMClient {
private:
    std::string modelName_;
    std::string baseUrl_;     // dùng ở chế độ (1); rỗng nếu dùng chế độ Gist
    std::string gistId_;      // dùng ở chế độ (2); rỗng nếu dùng baseUrl trực tiếp
    std::string gistPAT_;
    float temperature_ = 0.7f;   // có giá trị mặc định hợp lý, KHÔNG còn để rác chưa khởi tạo
    int maxTokens_ = 2048;

    // === BỔ SUNG: lưu tổng token (prompt_eval_count + eval_count) mà Ollama
    // trả về ở lần gọi processRequest() gần nhất, để AgentLoop đọc lại qua
    // getLastTokenUsage() sau mỗi bước và ghi đúng vào Step::tokensUsed.
    int lastTokenUsage_ = 0;

    std::string encodeImageToBase64(const std::string& filePath);
    std::string buildPayload(const std::string& promptText, const std::string& base64Image);
    std::expected<std::string, LLMError> executeCurl(const std::string& url,
                                                       const std::string& method,
                                                       const std::string& payload);
    std::expected<std::string, LLMError> resolveEndpoint();
    std::expected<std::string, LLMError> processRequest(const std::string& jsonPayload);

    // Constructor đầy đủ dùng nội bộ bởi cả 2 cách khởi tạo công khai bên dưới.
    OllamaClient(std::string model, std::string baseUrl, std::string gistId, std::string gistPAT);

public:
    // Chế độ 1: base URL cố định/local, vd "http://localhost:11434".
    OllamaClient(std::string model, std::string baseUrl);

    // Chế độ 2: dò URL động qua Gist (named constructor, tránh nhầm tham số với chế độ 1).
    static OllamaClient withGistDiscovery(std::string model, std::string gistId, std::string gistPAT);

    std::expected<std::string, LLMError> sendPrompt(const std::string& text) override;
    std::expected<std::string, LLMError> sendMessages(const std::vector<Message>& messages) override;
    std::expected<std::string, LLMError> sendPromptWithImage(
        const std::string& text, const std::string& imagePath) override;

    void setTemperature(float temperature) override { temperature_ = temperature; }
    void setMaxTokens(int maxTokens) override { maxTokens_ = maxTokens; }
    std::string getModelName() const override { return modelName_; }
    int getLastTokenUsage() const override { return lastTokenUsage_; }
};
