---
keywords: ke hoach, kế hoạch, danh sach task, danh sách task, todo, viec can lam, việc cần làm, chia nho task, chia nhỏ task, cac buoc, các bước, lap ke hoach, lập kế hoạch, todo_list, quan ly task, quản lý task, nhieu buoc, nhiều bước, tung buoc, từng bước, danh sach cong viec, danh sách công việc
---
## CÔNG CỤ: todo_list

Dùng tool này khi task YÊU CẦU NHIỀU BƯỚC (từ 3 bước trở lên, hoặc cần tự
quyết định thứ tự thực hiện) — giúp bạn không bị quên bước nào và có thể
theo dõi tiến độ rõ ràng.

KHÔNG dùng tool này cho task đơn giản (1-2 bước, ví dụ chỉ tính toán rồi trả
lời ngay) — sẽ làm task chậm không cần thiết.

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

Thêm task con:
```
Thought: Task nay co 2 buoc: tinh toan va luu file. Can lap ke hoach truoc.
Action: todo_list
Arguments: add | Tinh (25+15)*2 bang calculator
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

### 4. Lỗi thường gặp cần tránh

- Không gọi `start`/`done`/`remove` với id chưa từng `add` — tool sẽ báo lỗi
  "khong tim thay task".
- Không quên gọi `done` sau khi hoàn thành — nếu để task ở trạng thái
  `in_progress` mãi, người chấm sẽ không biết agent đã thực sự xong hay chưa.