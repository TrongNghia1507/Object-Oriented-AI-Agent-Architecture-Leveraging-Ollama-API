---
keywords: dich, dịch, translate, phien dich, phiên dịch, dich thuat, dịch thuật, dich sang, dịch sang, dich tu, dịch từ, dich nghia, dịch nghĩa
---
# Kỹ năng Dịch thuật (Translate Skill)

## 1. Giới thiệu công cụ khả dụng
Bạn có quyền truy cập vào công cụ dịch thuật sau:
- `translate`: Dịch văn bản qua dịch vụ MyMemory (miễn phí, không cần API key).
  Mặc định dịch **Tiếng Việt -> Tiếng Anh**, nhưng có thể chỉ định ngôn ngữ khác.

## 2. MỤC ĐÍCH QUAN TRỌNG NHẤT: Hỗ trợ tool `web_search`
Công cụ `web_search` (DuckDuckGo Instant Answer API) hoạt động RẤT KÉM với tiếng Việt.
Vì vậy, **BẮT BUỘC** phải dùng `translate` để dịch từ khóa sang Tiếng Anh
TRƯỚC KHI gọi `web_search`, thay vì tự dịch bằng "trí nhớ" của bạn.

Quy trình chuẩn (2 bước, mỗi bước là MỘT Action riêng biệt — nhớ luật
"Single Step & Wait" ở general_assistant.md):

**Bước 1 — Dịch từ khóa:**
```
Thought: Cau hoi cua nguoi dung la tieng Viet, can dich sang tieng Anh truoc khi tim kiem.
Action: translate
Arguments: <câu hỏi hoặc từ khóa tiếng Việt, đã rút gọn còn danh từ cốt lõi>
```
Hệ thống trả về Observation dạng: `Ban dich (vi->en): <ket qua tieng Anh>`

**Bước 2 — Tìm kiếm bằng bản dịch:**
```
Thought: Da co ban dich tieng Anh, gio tien hanh tim kiem.
Action: web_search
Arguments: <đúng phần bản dịch tiếng Anh lấy từ Observation ở bước 1>
```

## 3. Định dạng tham số của `translate`
- `"<văn bản>"` → dịch Việt → Anh (mặc định, dùng cho trường hợp chuẩn bị gọi web_search).
- `"<văn bản> | <mã ngôn ngữ đích>"` → dịch từ Tiếng Việt sang ngôn ngữ chỉ định.
- `"<văn bản> | <mã ngôn ngữ nguồn> | <mã ngôn ngữ đích>"` → chỉ định rõ cả hai chiều.
- Mã ngôn ngữ dùng chuẩn ISO 639-1: `vi`, `en`, `fr`, `ja`, `ko`, `zh-CN`, ...

## 4. Khi nào dùng `translate` ĐỘC LẬP (không kèm web_search)
Nếu người dùng yêu cầu trực tiếp việc dịch văn bản (ví dụ: "dịch câu này sang tiếng Anh giúp tôi", "câu 'xin chào' tiếng Nhật là gì?"), hãy gọi thẳng `translate` và trả lời
bằng `Final Answer` ngay sau khi có Observation, KHÔNG cần gọi thêm `web_search`.

## 5. Ví dụ

Người dùng hỏi: "Thủ đô của nước Pháp là gì?"
```
Thought: Can dich cau hoi sang tieng Anh truoc khi tim kiem tren web_search.
Action: translate
Arguments: Thủ đô của nước Pháp
```
(Observation: `Ban dich (vi->en): What is the capital of France`)
```
Thought: Da co ban dich, tien hanh tim kiem.
Action: web_search
Arguments: capital of France
```

Người dùng hỏi: "Dịch giúp tôi câu 'Tôi yêu lập trình' sang tiếng Anh"
```
Thought: Day la yeu cau dich truc tiep, khong can web_search.
Action: translate
Arguments: Tôi yêu lập trình
```
(Observation: `Ban dich (vi->en): I love programming`)
```
Final Answer: "Tôi yêu lập trình" dịch sang tiếng Anh là: "I love programming".
```
