---
keywords: tinh, toan, tính, toán, phep tinh, phép tính, bieu thuc, biểu thức, calculator, advanced_math, cong, tru, nhan, chia, cộng, trừ, nhân, chia, sqrt, can bac hai, căn bậc hai, luy thua, lũy thừa, sin, cos, tan, log, luong giac, lượng giác, pi
---
# Kỹ năng Định tuyến và Giải toán Máy tính (Math Routing Skill)

## 1. Giới thiệu công cụ khả dụng
Bạn có quyền truy cập vào hai công cụ tính toán chuyên biệt sau:
- `calculator`: Công cụ tính toán 4 phép tính cơ bản (+, -, *, /) hỗ trợ dấu ngoặc lồng nhau.
- `advanced_math`: Công cụ xử lý toán học cao cấp như căn bậc hai, lũy thừa, lượng giác, và các hằng số toán học.

## 2. Nguyên tắc định tuyến hành động (Action Routing)
Khi nhận yêu cầu toán học từ người dùng, bạn phải suy nghĩ (Think) và lựa chọn công cụ chính xác dựa theo quy tắc:

- **TRƯỜNG HỢP 1**: Biểu thức CHỈ chứa các số (nguyên hoặc thập phân), dấu ngoặc `()`, và các toán tử cơ bản `+`, `-`, `*`, `/`.
  => BẮT BUỘC gọi công cụ `calculator`.
  * Ví dụ tham số truyền vào: `(15 + 5) * 17` hoặc `34.5 / (2.0 - 0.5)`

- **TRƯỜNG HỢP 2**: Biểu thức xuất hiện các từ khóa/hàm nâng cao như căn bậc hai (`sqrt`), lũy thừa (`^` hoặc `pow`), lượng giác (`sin`, `cos`, `tan`), logarit (`log`, `log10`), hoặc các hằng số (`pi`, `e`).
  => BẮT BUỘC gọi công cụ `advanced_math`.
  * Ví dụ tham số truyền vào: `sqrt(144) + 2^3` hoặc `sin(pi / 2) * log(100)`



## 3. Định dạng đầu ra ra lệnh (Tool Call Format)
Bạn BẮT BUỘC phải xuất chuỗi hành động theo đúng định dạng JSON để hệ thống tự động bóc tách. TUYỆT ĐỐI KHÔNG dùng định dạng Thought/Action bằng chữ. 

Cấu trúc JSON bắt buộc:
{
  "action": "[Tên công cụ]",
  "arguments": "[Chuỗi biểu thức toán học sạch]"
}

Ví dụ ĐÚNG cho Toán cơ bản:
{
  "action": "calculator",
  "arguments": "(15+5)*17"
}

Ví dụ ĐÚNG cho Toán nâng cao:
{
  "action": "advanced_math",
  "arguments": "sqrt(144) + 2^3"
}