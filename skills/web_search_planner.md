---
keywords: web, search, internet, tim kiem, tìm kiếm, tra cuu, tra cứu, duckduckgo, hien tai, hiện tại, hom nay, hôm nay, tin tuc, tin tức, ai la, ai là, cai gi, cái gì
---
# Kỹ năng Tra cứu Thông tin Mạng (Web Search Skill)

## 1. Giới thiệu công cụ khả dụng
Bạn có quyền truy cập vào công cụ tìm kiếm trực tuyến sau để tra cứu thông tin theo thời gian thực:
- `web_search`: Công cụ gửi truy vấn tìm kiếm lên mạng Internet (thông qua hệ thống tìm kiếm DuckDuckGo) để lấy về các trích đoạn thông tin mới nhất.

## 2. Nguyên tắc định tuyến hành động (Action Routing)
Khi tương tác với người dùng, hãy đánh giá xem câu hỏi có mang tính thời sự, thực tế khách quan hay đòi hỏi dữ kiện chính xác mà bạn không chắc chắn hay không. Hãy tuân thủ quy tắc sau:

- **TRƯỜNG HỢP SỬ DỤNG (`web_search`)**:
  * Khi người dùng hỏi về các sự kiện, nhân vật, định nghĩa, địa lý, lịch sử, hoặc tin tức ("Ai là...", "Thủ đô của...", "Sự kiện X diễn ra khi nào?").
  * **QUY TẮC TẠO TỪ KHÓA (CỰC KỲ QUAN TRỌNG)**:
    1. Cỗ máy tìm kiếm hoạt động rất kém với tiếng Việt. Bạn BẮT BUỘC phải dịch từ khóa sang **Tiếng Anh** trước khi gọi tool.
    2. Từ khóa BẮT BUỘC phải là một danh từ chuẩn xác (tên riêng, địa danh, thuật ngữ khoa học), loại bỏ toàn bộ các từ nối, từ để hỏi dư thừa (như "là gì", "thủ đô", "ai là", "thế bạn").

## 3. Định dạng đầu ra ra lệnh (Tool Call Format)
Bạn BẮT BUỘC phải xuất chuỗi hành động theo đúng định dạng JSON để hệ thống C++ bóc tách cú pháp một cách tự động. 

Cấu trúc JSON bắt buộc:
{
  "action": "web_search",
  "arguments": "[Từ khóa Tiếng Anh đã được rút gọn]"
}

Ví dụ ĐÚNG (Khi người dùng hỏi: "Thủ đô của nước Pháp là gì?"):
{
  "action": "web_search",
  "arguments": "France"
}

Ví dụ ĐÚNG (Khi người dùng hỏi: "Cho tôi biết thông tin về Albert Einstein?"):
{
  "action": "web_search",
  "arguments": "Albert Einstein"
}