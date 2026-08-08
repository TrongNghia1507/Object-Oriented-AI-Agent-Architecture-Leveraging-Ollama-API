#pragma once
#include "tool.h"
#include <string>
#include <vector>

// TranslateTool — dịch văn bản qua MyMemory Translation API (miễn phí, không cần API key).
//
// MỤC ĐÍCH CHÍNH: hỗ trợ WebSearchTool. WebSearchTool (DuckDuckGo Instant Answer API)
// hoạt động rất kém với tiếng Việt, nên Agent nên gọi TranslateTool để dịch từ khóa
// sang Tiếng Anh TRƯỚC KHI gọi web_search. Quy trình 2 bước gợi ý:
//   Bước 1) Action: translate      | Arguments: "<câu hỏi/từ khóa tiếng Việt>"
//   Bước 2) Action: web_search     | Arguments: "<bản dịch tiếng Anh nhận được ở Observation>"
//
// Định dạng arguments (phân tách bằng dấu '|', giống quy ước của MemorySaveTool):
//   "<text>"                          -> dịch vi -> en (mặc định, dùng cho web_search)
//   "<text> | <target_lang>"          -> dịch auto-detect nguồn (mặc định vi) -> target_lang
//   "<text> | <source_lang> | <target_lang>" -> chỉ định rõ cả 2 chiều
// Mã ngôn ngữ dùng chuẩn ISO 639-1 viết tắt: vi, en, fr, ja, zh-CN, ko, ...
class TranslateTool : public Tool {
public:
    TranslateTool() = default;
    ~TranslateTool() override = default;

    std::string getName() const override { return "translate"; }
    std::string getDescription() const override {
        return "Dich van ban giua cac ngon ngu (qua MyMemory API, mien phi, khong can key). "
               "Dung tool nay de dich tu khoa Tieng Viet sang Tieng Anh TRUOC KHI goi "
               "tool 'web_search' (vi web_search hoat dong kem voi Tieng Viet). "
               "Tham so: '<van ban>' (mac dinh dich vi->en), hoac "
               "'<van ban> | <ma ngon ngu dich>', hoac "
               "'<van ban> | <ma ngon ngu goc> | <ma ngon ngu dich>'. "
               "Vi du: 'Thap Eiffel cao bao nhieu' -> tra ve ban dich Tieng Anh.";
    }

    // Nhận chuỗi thuần theo định dạng mô tả ở trên (AgentLoop đã bóc tách trước).
    std::string execute(const std::string& arguments) override;

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::string urlEncode(const std::string& value);

    // Gọi MyMemory API. Trả về "" nếu lỗi (chi tiết lỗi ghi vào errorOut).
    std::string callTranslateApi(const std::string& text,
                                  const std::string& sourceLang,
                                  const std::string& targetLang,
                                  std::string& errorOut);

    // Tách "text | a | b" thành tối đa 3 phần theo dấu '|', trim khoảng trắng từng phần.
    static std::vector<std::string> splitPipe(const std::string& s);
    static std::string trim(const std::string& s);
};
