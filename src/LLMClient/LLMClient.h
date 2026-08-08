#pragma once
#include <expected>
#include <string>
#include <vector>

enum class LLMError {
    Timeout,
    ConnectionDenied,
    MalformedJson,
    UnknownNetworkError
};

// Đại diện 1 turn trong conversation history (format /api/chat của Ollama).
// AgentLoop xây dựng vector<Message> qua nhiều bước rồi gửi 1 lần.
struct Message {
    std::string role;    // "system" | "user" | "assistant"
    std::string content;
    std::string image;
    bool hasImage = false;
};

// Interface trừu tượng cho tầng LLM Client .
// AgentLoop và HarnessRunner chỉ phụ thuộc vào interface này —
// thay Ollama bằng OpenAI chỉ cần viết thêm 1 class mới.
class LLMClient {
public:
    virtual ~LLMClient() = default;

    // Single-turn (tiện lợi, không duy trì history).
    virtual std::expected<std::string, LLMError>
        sendPrompt(const std::string& text) = 0;

    // Multi-turn: gửi toàn bộ conversation history.
    // AgentLoop gọi method này ở mỗi bước ReAct để model thấy đủ context.
    virtual std::expected<std::string, LLMError>
        sendMessages(const std::vector<Message>& messages) = 0;

    // Multimodal (text + ảnh), cùng interface với sendPrompt (yêu cầu 3.1).
    virtual std::expected<std::string, LLMError>
        sendPromptWithImage(const std::string& text,
                            const std::string& imagePath) = 0;

    virtual void setTemperature(float temperature) = 0;
    virtual void setMaxTokens(int maxTokens) = 0;
    virtual std::string getModelName() const = 0;
    virtual int getLastTokenUsage() const = 0;
};
