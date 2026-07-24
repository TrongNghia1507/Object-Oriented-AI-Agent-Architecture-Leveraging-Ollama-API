---
keywords: web, search, internet, tim kiem, tìm kiếm, tra cuu, tra cứu, searxng, google, hien tai, hiện tại, hom nay, hôm nay, tin tuc, tin tức, ai la, ai là, cai gi, cái gì
---
# Kỹ năng Tra cứu Thông tin Mạng (Web Search Skill)

## 1. Giới thiệu công cụ khả dụng
Bạn có quyền truy cập vào công cụ tìm kiếm trực tuyến sau để tra cứu thông tin theo thời gian thực:
- `web_search`: Công cụ gửi truy vấn tìm kiếm lên mạng Internet (thông qua hệ thống SearXNG tổng hợp kết quả từ Google, Bing,...) để lấy về danh sách các bài viết, tiêu đề và trích đoạn nội dung chi tiết nhất.
- `translate`: Công cụ dịch văn bản (xem chi tiết ở `translate_planner.md`). Dùng để dịch từ khóa Tiếng Việt sang Tiếng Anh TRƯỚC KHI gọi `web_search`, thay vì tự dịch bằng kiến thức nội tại — kết quả tìm kiếm bằng tiếng Anh qua tool luôn đầy đủ và chính xác hơn.

## 2. Nguyên tắc định tuyến hành động (Action Routing)
Khi tương tác với người dùng, hãy đánh giá xem câu hỏi có mang tính thời sự, thực tế khách quan hay đòi hỏi dữ kiện chính xác mà bạn không chắc chắn hay không. Hãy tuân thủ quy tắc sau:

- **TRƯỜNG HỢP SỬ DỤNG (`web_search`)**:
  * Khi người dùng hỏi về các sự kiện, nhân vật, định nghĩa, địa lý, lịch sử, hoặc tin tức ("Ai là...", "Tình hình kinh tế...", "Sự kiện X diễn ra khi nào?").
  * **QUY TẮC TẠO TỪ KHÓA (CỰC KỲ QUAN TRỌNG)**:
    1. Để tối ưu hóa công cụ tìm kiếm, bạn BẮT BUỘC phải gọi tool `translate` để dịch câu hỏi/từ khóa sang **Tiếng Anh** TRƯỚC KHI gọi `web_search` (xem quy trình 2 bước chi tiết ở `translate_planner.md`). KHÔNG tự dịch bằng kiến thức nội tại.
    2. Từ khóa cần súc tích và tập trung vào thực thể (tên riêng, địa danh, mốc thời gian, thuật ngữ khoa học). Hãy loại bỏ toàn bộ các từ nối, từ để hỏi dư thừa (như "là gì", "cho tôi biết", "thế bạn").

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
  "arguments": "capital of France"
}

Ví dụ ĐÚNG (Khi người dùng hỏi: "Tình hình lạm phát Việt Nam năm nay?"):
{
  "action": "web_search",
  "arguments": "Vietnam inflation rate current year"
}