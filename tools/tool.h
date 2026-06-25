#pragma once
#include <string>

// Interface trừu tượng (pure virtual) cho mọi Tool.
// Quy tắc thiết kế: Tool KHÔNG biết JSON, KHÔNG biết LLM protocol.
// AgentLoop chịu trách nhiệm bóc tách arguments từ response của model
// trước khi gọi execute() — tool chỉ nhận chuỗi kết quả cuối cùng.
class Tool {
public:
    virtual ~Tool() = default;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;

    // Nhận chuỗi arguments thuần (ví dụ: "(15+5)*17" hay "[tag] | nội dung").
    // Trả về chuỗi kết quả để AgentLoop đưa vào Observation.
    virtual std::string execute(const std::string& arguments) = 0;
};
