---
keywords: nho, nhớ, tri nho, trí nhớ, luu, lưu, ghi nho, ghi nhớ, nhac lai, nhắc lại, tim lai, tìm lại, memory, ket qua truoc, kết quả trước, vua nay, vừa nãy, luc nay, lúc nãy, truoc do, trước đó
---
# Kỹ năng Quản lý Trí nhớ và Truy vết Bối cảnh (Memory & Context Management Skill)

## 1. Giới thiệu công cụ khả dụng
Bạn có quyền truy cập vào hai công cụ quản lý bộ nhớ dài hạn chuyên biệt sau để lưu giữ thông tin xuyên phiên hội thoại:
- `memory_save`: Công cụ lưu trữ thông tin quan trọng, kết quả tính toán hoặc bối cảnh cuộc gọi vào Cơ sở dữ liệu SQLite dưới cấu trúc phân loại nhãn (Tag).
- `memory_search`: Công cụ tìm kiếm dữ liệu đã lưu trong quá khứ thông qua từ khóa để tái tạo lại bộ nhớ bối cảnh.

## 2. Nguyên tắc định tuyến hành động (Action Routing)
Khi tương tác với người dùng, bạn phải liên tục đánh giá xem thông tin có cần được ghi nhớ lâu dài hay cần được lục lại từ quá khứ hay không. Hãy lựa chọn công cụ dựa theo quy tắc:

- **TRƯỜNG HỢP 1 (Lưu trữ - `memory_save`)**: Khi xuất hiện một kết quả tính toán quan trọng từ công cụ toán học, một thông tin cá nhân của người dùng, hoặc một tình huống bối cảnh thực tế cần lưu lại cho các phiên làm việc sau này.
  * **Định dạng tham số bắt buộc**: `[TÊN_NHÃN] | <Nội dung chi tiết muốn lưu>`
  * **Bảng quy ước nhãn hệ thống (System Tags)**:
    - `math_result`: Dành cho các con số kết quả, công thức toán học vừa giải xong.
    - `scenario`: Dành cho các câu chuyện thực tế, đề bài toán mẫu hoặc bối cảnh mô phỏng.
    - `personal_info`: Dành cho tên, sở thích, yêu cầu cố định của người dùng.
    - `general`: Dành cho các thông tin chung khác.
  * *Ví dụ cụ thể*:
    - `Arguments: [math_result] | Kết quả phép tính 15 * 3 là 45`
    - `Arguments: [scenario] | Tình huống thực tế: Đi siêu thị mua 45 quả táo`

- **TRƯỜNG HỢP 2 (Tìm kiếm - `memory_search`)**: Khi người dùng sử dụng các đại từ chỉ định bối cảnh cũ như "kết quả toán vừa nãy", "tình huống trên", "mật khẩu hôm trước là gì" khi hệ thống vừa khởi động lại (Context Window trống), hoặc yêu cầu truy xuất thông tin cũ.
  * **Định dạng tham số**: Chuỗi từ khóa (keyword) sạch, tập trung vào cốt lõi bối cảnh cần tìm kiếm. Không truyền nhãn hoặc ký tự đặc biệt vào đây.
  * *Ví dụ cụ thể*:
    - `Arguments: 15 * 3` (Để tìm lại kết quả phép toán)
    - `Arguments: mua quả táo` (Để tìm lại tình huống thực tế liên quan)

## 3. Định dạng đầu ra ra lệnh (Tool Call Format)
Bạn phải xuất chuỗi hành động theo đúng định dạng khung ReAct (Thought-Action-Arguments) để Agent Loop của hệ thống C++ bóc tách cú pháp một cách tự động:

Thought: [Lý do cần sử dụng bộ nhớ, ví dụ: Người dùng yêu cầu lấy kết quả toán cũ cộng thêm 1, bối cảnh hiện tại không có nên tôi cần tìm lại kết quả phép tính 15 * 3 trong DB]
Action: [memory_save HOẶC memory_search]
Arguments: [Chuỗi tham số tuân thủ chính xác quy tắc ở Mục 2]
