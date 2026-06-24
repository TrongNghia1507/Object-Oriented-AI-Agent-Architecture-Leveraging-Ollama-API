---
keywords: web, search, internet, tim kiem, tìm kiếm, tra cuu, tra cứu, google, wikipedia, wiki, hien tai, hiện tại, hom nay, hôm nay, tin tuc, tin tức, ai la, ai là, cai gi, cái gì
---
# Kỹ năng Tra cứu Thông tin Mạng (Web Search Skill)

## 1. Giới thiệu công cụ khả dụng
Bạn có quyền truy cập vào công cụ tìm kiếm trực tuyến để tra cứu thông tin theo thời gian thực:
- `web_search`: Công cụ gửi truy vấn tìm kiếm lên mạng Internet (thông qua hệ thống tìm kiếm DuckDuckGo) để lấy về các trích đoạn thông tin mới nhất, khách quan nhất nhằm trả lời cho các câu hỏi mà kiến thức nội tại của bạn không có hoặc đã lỗi thời.

## 2. Nguyên tắc định tuyến hành động (Action Routing)
Khi tương tác với người dùng, hãy đánh giá xem câu hỏi có mang tính thời sự, thực tế khách quan hay đòi hỏi dữ kiện chính xác mà bạn không chắc chắn hay không.

- **TRƯỜNG HỢP SỬ DỤNG (`web_search`)**:
  * Khi người dùng hỏi về các sự kiện, nhân vật, định nghĩa, địa lý, lịch sử, hoặc tin tức ("Ai là...", "Thủ đô của...", "Sự kiện X diễn ra khi nào?").
  * **Định dạng tham số bắt buộc**: Chuỗi từ khóa tìm kiếm (keyword) ngắn gọn, súc tích, đã được lược bỏ các từ nối không cần thiết. Không nhét toàn bộ câu hỏi dài dòng vào tham số.
  * *Ví dụ cụ thể*:
    - Người dùng: "Tổng thống của nước Pháp hiện nay là ai?"
    - `Arguments: Tổng thống nước Pháp` (ĐÚNG)
    - `Arguments: Ai là tổng thống của nước Pháp hiện nay thế bạn?` (SAI - Quá dài)

## 3. Định dạng đầu ra ra lệnh (Tool Call Format)
Bạn phải xuất chuỗi hành động theo đúng định dạng khung ReAct (Thought-Action-Arguments) để Agent Loop của hệ thống C++ bóc tách cú pháp một cách tự động:

Thought: [Lý do cần sử dụng công cụ tìm kiếm, ví dụ: Người dùng hỏi về thông tin thực tế khách quan, tôi cần dùng web_search để tra cứu thông tin chính xác nhất thay vì tự bịa ra]
Action: web_search
Arguments: [Từ khóa tìm kiếm súc tích tuân thủ chính xác quy tắc ở Mục 2]