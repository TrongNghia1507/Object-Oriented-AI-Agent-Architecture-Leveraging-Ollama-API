---
keywords: nho, nhớ, tri nho, trí nhớ, luu, lưu, ghi nho, ghi nhớ, nhac lai, nhắc lại, tim lai, tìm lại, memory, ket qua truoc, kết quả trước, vua nay, vừa nãy, luc nay, lúc nãy, truoc do, trước đó
---

# Kỹ năng Quản lý Trí nhớ và Truy vết Bối cảnh (Memory & Context Management Skill)

## 1. Giới thiệu công cụ khả dụng

Bạn có quyền truy cập vào hai công cụ quản lý bộ nhớ dài hạn chuyên biệt (Persistent Memory) sử dụng **Vector Embedding**, **Cosine Similarity**, **Context Tag Filtering** và **Similarity + Recency Ranking** để lưu giữ và truy xuất thông tin xuyên suốt nhiều phiên hội thoại.

- `memory_save`: Công cụ lưu trữ thông tin quan trọng vào cơ sở dữ liệu SQLite. Hệ thống sẽ tự động:
  - Sinh embedding từ nội dung bằng mô hình embedding.
  - Chuẩn hóa (Normalize) embedding.
  - Lưu `context_tag`, `content`, `embedding` và `created_at`.

- `memory_search`: Công cụ tìm kiếm ngữ nghĩa (Semantic Search) trên bộ nhớ đã lưu. Hệ thống sẽ:
  - Sinh embedding cho câu truy vấn.
  - Chuẩn hóa embedding.
  - Lọc theo `context_tag` (nếu được chỉ định).
  - Tính Cosine Similarity giữa truy vấn và các memory.
  - Kết hợp Similarity Score với Recency Score.
  - Trả về Top-K kết quả phù hợp nhất.

**Lưu ý**

- `memory_search` là **Semantic Search**, không phải tìm kiếm theo từ khóa chính xác.
- Agent **không cần** tự tạo embedding, similarity hay score.
- Agent chỉ cần truyền đúng `Arguments`, mọi xử lý còn lại do Memory Tool thực hiện.

---

## 2. Nguyên tắc định tuyến hành động (Action Routing)

Khi tương tác với người dùng, bạn phải liên tục đánh giá xem thông tin có cần được ghi nhớ lâu dài hay cần được truy xuất từ bộ nhớ hay không.

### **TRƯỜNG HỢP 1 (Lưu trữ - `memory_save`)**

Sử dụng khi:

- Có kết quả tính toán quan trọng.
- Có thông tin cá nhân của người dùng.
- Có sở thích hoặc yêu cầu lâu dài của người dùng.
- Có kiến thức hoặc ghi chú mà người dùng yêu cầu lưu.
- Có bối cảnh cần sử dụng lại ở các phiên làm việc sau.

**Không sử dụng `memory_save` khi:**

- Chỉ là lời chào hoặc hội thoại xã giao.
- Nội dung chỉ phục vụ cho lượt hội thoại hiện tại.
- Thông tin có thể dễ dàng tính toán hoặc suy luận lại.
- Những thông tin không có giá trị lâu dài.

**Nếu không chắc chắn thông tin có nên lưu hay không:**

Không gọi `memory_save` ngay.

Ưu tiên hỏi lại người dùng để xác nhận trước khi lưu.

Ví dụ:

> "Bạn có muốn mình ghi nhớ thông tin này cho các cuộc trò chuyện sau không?"

### Định dạng tham số bắt buộc

```
[TÊN_NHÃN] | <Nội dung chi tiết muốn lưu>
```

### Bảng quy ước nhãn hệ thống (System Tags)

- `math_result`: Kết quả hoặc công thức toán học.
- `scenario`: Tình huống thực tế hoặc bối cảnh mô phỏng.
- `personal_info`: Thông tin cá nhân, sở thích hoặc yêu cầu lâu dài của người dùng.
- `coding`: Kiến thức lập trình.
- `database`: Kiến thức về SQL hoặc CSDL.
- `general`: Thông tin chung.

Nếu không xác định được nhãn phù hợp, sử dụng:

```
[general]
```

### Ví dụ

```
Arguments:
[math_result] | Kết quả phép tính 15 * 3 là 45
```

```
Arguments:
[personal_info] | Người dùng thích laptop ROG
```

```
Arguments:
[coding] | Thuật toán DFS sử dụng Stack để duyệt đồ thị
```

```
Arguments:
[database] | INNER JOIN trả về các bản ghi khớp ở cả hai bảng
```

---

### **TRƯỜNG HỢP 2 (Tìm kiếm - `memory_search`)**

Sử dụng khi:

- Người dùng muốn nhắc lại thông tin đã lưu.
- Người dùng hỏi về kết quả trước đó.
- Người dùng đề cập đến một bối cảnh cũ.
- Context Window hiện tại không còn chứa thông tin cần thiết.
- Người dùng yêu cầu truy xuất thông tin từ các phiên trước.

### Định dạng tham số bắt buộc

```
[TÊN_NHÃN] | <Mô tả ngắn gọn điều cần tìm>
```

**Lưu ý**

- Không cần sử dụng đúng từ khóa đã lưu.
- Có thể diễn đạt bằng ngôn ngữ tự nhiên.
- Không truyền embedding.
- Không truyền similarity.
- Không truyền score.
- Không truyền thời gian.

Nếu không biết nhãn phù hợp hoặc muốn tìm trên toàn bộ bộ nhớ:

```
[general] | <Mô tả cần tìm>
```

### Ví dụ

```
Arguments:
[math_result] | kết quả phép nhân vừa tính
```

```
Arguments:
[scenario] | bài toán mua táo
```

```
Arguments:
[personal_info] | sở thích laptop
```

```
Arguments:
[coding] | thuật toán DFS
```

```
Arguments:
[database] | SQL JOIN
```

```
Arguments:
[general] | mật khẩu wifi
```

---

## 3. Định dạng đầu ra ra lệnh (Tool Call Format)

Bạn phải xuất chuỗi hành động theo đúng định dạng khung ReAct (Thought - Action - Arguments) để Agent Loop của hệ thống C++ có thể phân tích và thực thi tự động.

```
Thought:
<Lý do cần sử dụng bộ nhớ>
```

```
Action:
memory_save
```

hoặc

```
Action:
memory_search
```

```
Arguments:
<Chuỗi tham số tuân thủ đúng quy tắc ở Mục 2>
```

### Ví dụ 1

```
Thought:
Người dùng muốn lưu sở thích cá nhân để sử dụng trong các phiên sau.

Action:
memory_save

Arguments:
[personal_info] | Người dùng thích laptop ROG
```

### Ví dụ 2

```
Thought:
Người dùng muốn lấy lại kết quả phép tính trước đó nhưng Context Window hiện tại không còn nên cần truy xuất Persistent Memory.

Action:
memory_search

Arguments:
[math_result] | kết quả phép nhân vừa tính
```

### Ví dụ 3

```
Thought:
Người dùng hỏi lại kiến thức SQL đã lưu.

Action:
memory_search

Arguments:
[database] | SQL JOIN
```

### Nguyên tắc quan trọng

- Chỉ lưu những thông tin có giá trị lâu dài.
- Không lưu toàn bộ hội thoại.
- Không lưu các câu hỏi tạm thời.
- Khi không chắc chắn có nên lưu hay không, hãy hỏi lại người dùng trước.
- Tin tưởng Semantic Search thay vì cố gắng nhớ chính xác từng từ khóa.
- Luôn chọn `context_tag` phù hợp để tăng chất lượng tìm kiếm.