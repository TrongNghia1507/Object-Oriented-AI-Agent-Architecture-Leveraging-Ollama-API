#ifndef TOOL_H
#define TOOL_H
#include "../LLMClient/json.hpp"
#include <string>
using json = nlohmann::json;
class Tool {
public:
    // Destructor ảo là BẮT BUỘC trong OOP để tránh rò rỉ bộ nhớ khi giải phóng con trỏ lớp cha
    virtual ~Tool() = default;

    // Trả về tên định danh duy nhất của công cụ (ví dụ: "calculator")
    virtual std::string getName() const = 0;

    // Trả về mô tả chức năng và hướng dẫn tham số để AI hiểu khi nào nên dùng
    virtual std::string getDescription() const = 0;

    // Hàm thực thi cốt lõi: nhận vào chuỗi đối số từ AI và trả về kết quả dạng chuỗi
    virtual std::string execute(const std::string& args) = 0;
};

#endif // TOOL_H