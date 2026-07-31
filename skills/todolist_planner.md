---
keywords: ke hoach, kế hoạch, danh sach task, danh sách task, todo, viec can lam, việc cần làm, chia nho task, chia nhỏ task, cac buoc, các bước, lap ke hoach, lập kế hoạch, todo_list, quan ly task, quản lý task, nhieu buoc, nhiều bước, tung buoc, từng bước, danh sach cong viec, danh sách công việc
---
## CÔNG CỤ: todo_list

### 0. QUY TẮC QUYẾT ĐỊNH (BẮT BUỘC làm trước, ngay trong Thought đầu tiên)

Trước khi hành động, tự đếm số LẦN GỌI TOOL riêng biệt cần thiết để hoàn
thành task (không tính chính todo_list):

- **Đếm được ≤ 2 lần gọi tool** (vd: chỉ `calculator` rồi trả lời; hoặc
  `calculator` xong `write_file`) → **TUYỆT ĐỐI KHÔNG gọi `todo_list`**. Thực
  hiện thẳng các tool cần thiết rồi viết `Final Answer`.
- **Đếm được ≥ 3 lần gọi tool khác nhau**, HOẶC không chắc thứ tự thực hiện,
  HOẶC có nhánh điều kiện (nếu... thì...) → gọi `todo_list` để lập kế hoạch
  trước khi làm.

Vi phạm quy tắc này (gọi todo_list cho task ≤2 bước) bị coi là dùng sai tool,
làm task chậm và tốn token không cần thiết — luôn ưu tiên đường đi ngắn nhất
khi task đơn giản.

### 1. Các lệnh (Arguments)

| Arguments | Tác dụng |
|---|---|
| `add \| noi dung task` | Thêm 1 task con mới |
| `start \| id` | Đánh dấu task đang thực hiện (chỉ 1 task in_progress cùng lúc) |
| `done \| id` | Đánh dấu task đã hoàn thành |
| `remove \| id` | Xoá task khỏi danh sách |
| (để trống) hoặc `list` | Xem toàn bộ danh sách hiện tại |

`id` là số nguyên do tool tự sinh khi `add`, được trả về trong Observation —
PHẢI đọc đúng số id đó, không tự đoán.

### 2. Quy trình khuyến nghị cho task nhiều bước

1. Bước đầu tiên: gọi `add` liên tiếp (mỗi lượt 1 task con) để liệt kê hết các
   việc cần làm trước khi bắt tay thực hiện.
2. Trước khi thực hiện 1 việc, gọi `start | id` để đánh dấu đang làm việc đó.
3. Dùng tool tương ứng (`calculator`, `write_file`, `exec`, ...) để thực sự
   thực hiện việc con đó.
4. Sau khi xong, gọi `done | id` rồi chuyển sang việc tiếp theo.
5. Chỉ viết `Final Answer` khi TẤT CẢ task con đã `done` — có thể gọi `list`
   (không tham số) để kiểm tra lại trước khi kết thúc.

### 3. Ví dụ đúng chuẩn (1 hành động mỗi lượt)

Thêm task con (task này cần 3 lần gọi tool: advanced_math, memory_save, exec
→ ĐỦ điều kiện dùng todo_list theo quy tắc ở mục 0):
```
Thought: Task nay can 3 lan goi tool khac nhau (advanced_math, memory_save,
exec) va agent phai tu quyet dinh thu tu. Can lap ke hoach truoc.
Action: todo_list
Arguments: add | Tinh sqrt(144) + 2^3 bang advanced_math
```

Bắt đầu thực hiện task #1:
```
Thought: Bat dau lam task dau tien trong danh sach.
Action: todo_list
Arguments: start | 1
```

Đánh dấu hoàn thành sau khi đã có kết quả:
```
Thought: Da tinh xong va co ket qua, danh dau task nay hoan thanh.
Action: todo_list
Arguments: done | 1
```

### 4. Ví dụ SAI cần tránh — task đơn giản KHÔNG được dùng todo_list

Task: "Tính 15 * 17 và cho biết kết quả." → chỉ cần 1 lần gọi `calculator`
rồi `Final Answer` ngay. KHÔNG được làm thế này:
```
Thought: Can lap ke hoach truoc khi tinh.
Action: todo_list
Arguments: add | Tinh 15*17
```
Làm ĐÚNG:
```
Thought: Day la phep tinh don gian, chi can 1 lan goi calculator, khong can
lap ke hoach.
Action: calculator
Arguments: 15*17
```

### 5. Lỗi thường gặp cần tránh

- Không gọi `start`/`done`/`remove` với id chưa từng `add` — tool sẽ báo lỗi
  "khong tim thay task".
- Không quên gọi `done` sau khi hoàn thành — nếu để task ở trạng thái
  `in_progress` mãi, người chấm sẽ không biết agent đã thực sự xong hay chưa.