---
keywords: web, search, internet, tim kiem, tìm kiếm, tra cuu, tra cứu, duckduckgo, hien tai, hiện tại, hom nay, hôm nay, tin tuc, tin tức, ai la, ai là, cai gi, cái gì
---
# Kỹ năng Tra cứu Thông tin Mạng (Web Search Skill)

## 1. Giới thiệu công cụ khả dụng
Bạn có quyền truy cập vào công cụ tìm kiếm trực tuyến sau để tra cứu thông tin theo thời gian thực:
- `web_search`: Công cụ gửi truy vấn tìm kiếm lên mạng Internet (thông qua hệ thống tìm kiếm DuckDuckGo) để lấy về các trích đoạn thông tin mới nhất.
- `translate`: Công cụ dịch văn bản (xem chi tiết ở `translate_planner.md`). Dùng để dịch
  từ khóa Tiếng Việt sang Tiếng Anh TRƯỚC KHI gọi `web_search`, thay vì tự dịch bằng
  kiến thức nội tại — kết quả dịch qua tool đáng tin cậy và nhất quán hơn.

## 2. Nguyên tắc định tuyến hành động (Action Routing)
Khi tương tác với người dùng, hãy đánh giá xem câu hỏi có mang tính thời sự, thực tế khách quan hay đòi hỏi dữ kiện chính xác mà bạn không chắc chắn hay không. Hãy tuân thủ quy tắc sau:

- **TRƯỜNG HỢP SỬ DỤNG (`web_search`)**:
  * Khi người dùng hỏi về các sự kiện, nhân vật, định nghĩa, địa lý, lịch sử, hoặc tin tức ("Ai là...", "Thủ đô của...", "Sự kiện X diễn ra khi nào?").
  * **QUY TẮC TẠO TỪ KHÓA (CỰC KỲ QUAN TRỌNG)**:
    1. Cỗ máy tìm kiếm hoạt động rất kém với tiếng Việt. Bạn BẮT BUỘC phải gọi tool
       `translate` để dịch từ khóa sang **Tiếng Anh** TRƯỚC KHI gọi `web_search`
       (xem quy trình 2 bước chi tiết ở `translate_planner.md`). KHÔNG tự dịch bằng
       kiến thức nội tại — luôn đi qua tool `translate` để đảm bảo tính nhất quán.
    2. Từ khóa BẮT BUỘC phải là một danh từ chuẩn xác (tên riêng, địa danh, thuật ngữ khoa học), loại bỏ toàn bộ các từ nối, từ để hỏi dư thừa (như "là gì", "thủ đô", "ai là", "thế bạn") — có thể rút gọn trước hoặc sau bước dịch.

## 3. Định dạng đầu ra ra lệnh (Tool Call Format)
Bạn BẮT BUỘC phải xuất hành động theo đúng 3 dòng chữ thuần (plain text) sau,
ĐÚNG NHƯ quy tắc ở cuối system prompt. TUYỆT ĐỐI KHÔNG dùng định dạng JSON.

Cấu trúc bắt buộc:
```
Thought: <ly do chon tu khoa nay>
Action: web_search
Arguments: <Tu khoa Tieng Anh da duoc rut gon>
```

Ví dụ ĐÚNG (Khi người dùng hỏi: "Thủ đô của nước Pháp là gì?"):
```
Thought: Can tim tu khoa Tieng Anh ve nuoc Phap, da dich qua tool translate.
Action: web_search
Arguments: France
```

Ví dụ ĐÚNG (Khi người dùng hỏi: "Cho tôi biết thông tin về Albert Einstein?"):
```
Thought: Ten rieng khong can dich, dung truc tiep lam tu khoa tim kiem.
Action: web_search
Arguments: Albert Einstein
```