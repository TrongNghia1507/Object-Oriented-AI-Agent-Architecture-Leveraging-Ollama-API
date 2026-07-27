---
keywords: gio hien tai, giờ hiện tại, ngay hien tai, ngày hiện tại, thoi gian hien tai, thời gian hiện tại, lay ngay, lấy ngày, lay gio, lấy giờ, hom nay la ngay may, hôm nay là ngày mấy, unix timestamp, get_datetime, ngay thang, ngày tháng, bay gio la may gio, bây giờ là mấy giờ, thoi diem hien tai, thời điểm hiện tại
---
## CÔNG CỤ: get_datetime

Tool này trả về ngày/giờ hiện tại của hệ thống. KHÔNG dùng tool `exec` với lệnh
`date` để lấy thời gian nếu `get_datetime` có sẵn — dùng đúng tool chuyên biệt
này để kết quả nhất quán và dễ chấm điểm.

### 1. Các chế độ tham số (Arguments) — chọn ĐÚNG 1 trong 4 giá trị sau

| Arguments | Kết quả trả về |
|---|---|
| (để trống) hoặc `full` | Ngày + giờ đầy đủ, dạng `YYYY-MM-DD HH:MM:SS` |
| `date` | Chỉ ngày, dạng `YYYY-MM-DD` |
| `time` | Chỉ giờ, dạng `HH:MM:SS` |
| `unix` | Unix timestamp (số giây kể từ 1970), dạng số nguyên |

KHÔNG dùng các từ khác ngoài 4 giá trị trên (ví dụ không dùng `ngay`, `hien_tai`,
`now`) — tool sẽ báo lỗi định dạng nếu Arguments không đúng 1 trong 4 từ khoá.

### 2. Ví dụ đúng chuẩn

Chỉ cần lấy ngày, không cần giờ:
```
Thought: Can lay ngay hien tai de luu vao file.
Action: get_datetime
Arguments: date
```

Cần cả ngày và giờ:
```
Thought: Can biet chinh xac thoi diem hien tai.
Action: get_datetime
Arguments: full
```

Cần timestamp để tính toán (ví dụ so sánh thời gian):
```
Thought: Can lay unix timestamp de kiem tra dieu kien.
Action: get_datetime
Arguments: unix
```

### 3. Lưu ý khi kết hợp với write_file

Nếu task yêu cầu lưu kết quả vào file, PHẢI gọi `get_datetime` trước, nhận kết
quả (Observation), rồi mới gọi `write_file` ở bước tiếp theo với ĐÚNG NGUYÊN
VĂN kết quả nhận được — không tự đoán hay tự viết lại ngày giờ.