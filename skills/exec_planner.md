---
keywords: exec, chay lenh, bash, shell, lenh, xoa file, tao file, tao thu muc, ls, mkdir, rm, cat, grep, cp, mv, chmod, ping, curl, python, run, execute, terminal, command
---
## CÔNG CỤ: exec
Công cụ này cho phép bạn thực thi các lệnh Terminal/Bash/Shell trực tiếp trên hệ thống máy tính ảo.

### 1. ĐỊNH DẠNG DỮ LIỆU ĐẦU VÀO (ARGUMENTS)
Bạn BẮT BUỘC phải xuất hành động theo đúng 3 dòng chữ thuần (plain text) sau,
ĐÚNG NHƯ quy tắc ở cuối system prompt. TUYỆT ĐỐI KHÔNG dùng định dạng JSON.
Dòng `Arguments` PHẢI chứa DUY NHẤT một câu lệnh shell/bash hợp lệ (không thêm
giải thích hay chữ nào khác vào cùng dòng đó — phần giải thích đã nằm sẵn ở
dòng `Thought`).

Ví dụ ĐÚNG:
```
Thought: Can xem danh sach file trong thu muc hien tai.
Action: exec
Arguments: ls -la
```

### 2. QUY TẮC SỐNG CÒN (BẮT BUỘC TUÂN THỦ)
Nếu vi phạm các quy tắc này, hệ thống sẽ bị treo hoặc sập:
* **CẤM Lệnh Tương Tác:** Tuyệt đối không sử dụng các lệnh yêu cầu người dùng nhập liệu từ bàn phím (ví dụ: `nano`, `vim`, `top`, `python` không có script, `apt-get` thiếu cờ `-y`).
* **Bẫy Mất Trí Nhớ (Stateless):** Mỗi lần gọi `exec` là một phiên terminal mới. Lệnh `cd` sẽ không có tác dụng cho lần gọi sau. Nếu muốn đổi thư mục rồi chạy lệnh, PHẢI dùng toán tử `&&` (Ví dụ: `cd /workspace && ./app`).
* **Tạo Thư Mục An Toàn:** Khi dùng `mkdir`, BẮT BUỘC phải kèm theo cờ `-p` để tránh lỗi thư mục cha chưa tồn tại (Ví dụ: `mkdir -p src/utils`).
* **Chống Tràn Bộ Nhớ:** TUYỆT ĐỐI KHÔNG dùng lệnh `cat` đối với các file chưa biết kích thước. Hãy dùng `head -n 20 <tên_file>` để đọc 20 dòng đầu, hoặc dùng `grep "<từ_khóa>"` để tìm kiếm nội dung cụ thể.
* ĐẶC BIỆT LƯU Ý VỀ DẤU GẠCH CHÉO CODESPACES/LINUX:
  - Dấu `/` ở ĐẦU chuỗi (Ví dụ: `/results`) đại diện cho ĐƯỜNG DẪN TUYỆT ĐỐI tại gốc hệ điều hành (root).
  - Thư mục dự án của bạn nằm ở CWD (Ví dụ: `/workspaces/AgentAI`).
  - TUYỆT ĐỐI KHÔNG ĐƯỢC thêm dấu `/` vào trước tên thư mục con nếu muốn di chuyển vào đó. 
    + SAI: `cd /results` (Hệ thống sẽ tìm thư mục results ở gốc ổ cứng và báo lỗi can't cd).
    + ĐÚNG: `cd results` hoặc `cd ./results`.