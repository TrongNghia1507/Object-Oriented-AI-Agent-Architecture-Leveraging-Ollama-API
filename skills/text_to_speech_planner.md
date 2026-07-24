---
keywords: doc, đoc, phat am, ph t a m, tts, text to speech, noi, n o i, giong noi, giong doc, am thanh, m th
---

# Công cụ Tổng hợp Giọng nói (Text-to-Speech Skill)

## 1. Giới thiệu
Bạn có quyền truy cập vào công cụ `text_to_speech` để chuyển đổi văn bản (text) thành âm thanh (speech) thông qua API của ElevenLabs.

## 2. Nguyên tắc sử dụng
- Khi người dùng yêu cầu đọc văn bản, hãy gọi công cụ này.
- **Model luôn sử dụng là `eleven_multilingual_v2`** (đã được cấu hình cứng trong code để đọc tốt tiếng Việt).
- Bạn có thể nhận diện các mã Voice ID của ElevenLabs (chuỗi 20 ký tự) nếu người dùng cung cấp.

## 3. Định dạng đầu ra (Tool Call Format)
BẠN BẮT BUỘC phải xuất chuỗi chính xác theo định dạng JSON như sau cho phần `arguments`:

```json
{
  "action": "text_to_speech",
  "arguments": "{\n  \"text\": \"[Văn bản cần đọc]\",\n  \"voice\": \"21m00Tcm4TlvDq8ikWAM\",\n  \"output_file\": \"output.mp3\"\n}"
}