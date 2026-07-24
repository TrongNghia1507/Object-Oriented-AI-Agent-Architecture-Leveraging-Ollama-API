---
keywords: doc file, đọc file, ghi file, viết file, luu file, lưu file, tao file, tạo file, noi dung file, nội dung file, luu vao file, lưu vào file, doc noi dung file, đọc nội dung file, ghi vao file, ghi ra file, read_file, write_file, file txt, doc du lieu tu file, đọc dữ liệu từ file
---
## CÔNG CỤ: read_file / write_file

Bạn có 2 tool thao tác file:
- `read_file`: đọc toàn bộ nội dung 1 file.
- `write_file`: ghi (đè) nội dung vào 1 file.

### 1. Định dạng tham số (Arguments)

- Với `read_file`: `Arguments` PHẢI CHỈ CHỨA đường dẫn file, không thêm chữ nào khác.
  Ví dụ đúng: `Arguments: data.txt`

- Với `write_file`: `Arguments` PHẢI theo đúng khuôn `duong_dan | noi_dung`,
  với dấu cách + `|` + dấu cách ở giữa (đúng 1 lần). Phần `noi_dung` là TOÀN BỘ
  phần còn lại sau dấu `|` — không tự thêm dấu ngoặc kép hay ký tự thừa.
  Ví dụ đúng: `Arguments: result.txt | 255`

  LƯU Ý QUAN TRỌNG: nếu `noi_dung` cần ghi cũng chứa chuỗi `" | "` thì tool sẽ
  cắt nhầm — tránh dùng ký tự `|` bên trong nội dung cần ghi.

### 2. Ví dụ đúng chuẩn (theo QUY TAC TRA LOI của hệ thống)

Đọc file:
```
Thought: Can doc noi dung file data.txt de biet con so ben trong.
Action: read_file
Arguments: data.txt
```

Ghi file:
```
Thought: Da tinh duoc ket qua la 255, can luu vao result.txt.
Action: write_file
Arguments: result.txt | 255
```

### 3. Lỗi thường gặp cần tránh

- Không thêm tiền tố kiểu `"path: data.txt"` vào Arguments của `read_file` —
  chỉ cần đúng đường dẫn.
- Không quên dấu cách hai bên `|` khi gọi `write_file` — thiếu dấu cách sẽ
  khiến tool báo lỗi định dạng.
- Nếu `read_file` báo lỗi không mở được file, kiểm tra lại đường dẫn có đúng
  không trước khi thử lại, tránh lặp lại cùng 1 lỗi nhiều lần (dễ bị
  LoopDetector phát hiện và dừng agent).