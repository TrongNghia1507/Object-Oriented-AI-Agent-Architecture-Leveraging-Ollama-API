#pragma once
#include <string>
#include <expected>

// Mã lỗi chung cho mọi LLM client (không gắn riêng với Ollama)
// để các client khác (OpenAIClient, ClaudeClient...) có thể tái dùng.
enum class LLMError {
    Timeout,
    ConnectionDenied,
    MalformedJson,
    UnknownNetworkError
};

// Interface trừu tượng cho tầng LLM Client (yêu cầu 3.1 / IV.4.1 trong đề bài).
// AgentLoop chỉ nên phụ thuộc vào interface này, KHÔNG phụ thuộc trực tiếp
// vào OllamaClient -> sau này đổi sang OpenAI/Claude chỉ cần viết thêm
// một class mới kế thừa LLMClient, không phải sửa AgentLoop.
class LLMClient {
public:
    virtual ~LLMClient() = default;

    // Gửi một prompt văn bản thuần, nhận về nội dung trả lời hoặc lỗi.
    virtual std::expected<std::string, LLMError> sendPrompt(const std::string& text) = 0;

    // Gửi prompt kèm 1 ảnh (multimodal) qua CÙNG một interface với sendPrompt,
    // đáp ứng yêu cầu "text-only và multimodal qua cùng một interface" (3.1).
    virtual std::expected<std::string, LLMError> sendPromptWithImage(
        const std::string& text, const std::string& imagePath) = 0;

    // Cấu hình runtime (yêu cầu 3.1: temperature, max_tokens phải cấu hình được)
    virtual void setTemperature(float temperature) = 0;
    virtual void setMaxTokens(int maxTokens) = 0;
    virtual std::string getModelName() const = 0;
};
