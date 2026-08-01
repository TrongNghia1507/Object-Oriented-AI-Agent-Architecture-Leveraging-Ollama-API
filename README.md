# AgentAI
Dự án xây dựng một AI Agent bằng C++, hỗ trợ:
* Gọi mô hình ngôn ngữ (LLM).
* Thực thi công cụ (Tools).
* Ghi nhớ ngữ cảnh (Memory).
* Tìm kiếm thông tin trên web.
* Chuyển văn bản thành giọng nói (Text To Speech).
* Chạy benchmark và đánh giá kết quả.
---
## Cấu trúc thư mục
```text
AgentAI-main/
│
├── agent/                  # Vòng lặp điều khiển Agent
├── environment/            # Môi trường thực thi
├── Harness/                # Bộ đánh giá và benchmark
├── LLMClient/              # Kết nối với mô hình LLM
├── tools/                  # Các công cụ mở rộng
├── utils/                  # Hàm tiện ích
├── benchmark/              # Dữ liệu benchmark
├── benchmark_summary/      # Kết quả tổng hợp
├── image/                  # Ảnh minh họa
├── results/                # Kết quả đầu ra
├── sandbox/                # Dữ liệu thử nghiệm
├── tests/                  # Chương trình kiểm thử
├── CMakeLists.txt
└── model.env
```
---
## Yêu cầu hệ thống
### Hệ điều hành
* Windows 10/11
* Ubuntu 22.04 trở lên
### Công cụ cần cài đặt
* CMake 3.10 trở lên
* Trình biên dịch hỗ trợ C++23
* Git
* SQLite3
* libcurl
---
## Cài đặt thư viện
### Ubuntu
```bash
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    libsqlite3-dev
```
### Windows (MSYS2)
```bash
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-curl
pacman -S mingw-w64-x86_64-sqlite3
```
---
## Thiết lập môi trường
Mở tệp:
```text
model.env
```
Sau đó điền các khóa API:
```env
OPENROUTER_API_KEY=your_api_key
TTS_API_KEY=your_api_key
```
---
## Biên dịch dự án
Di chuyển vào thư mục dự án:
```bash
cd AgentAI-main
```
Tạo thư mục build:
```bash
mkdir build
cd build
```
Khởi tạo CMake:
```bash
cmake ..
```
Biên dịch:
```bash
cmake --build .
```
---
## Chạy chương trình
### Chạy bộ kiểm thử
```bash
./harness_runner_test
```
---
## Một số tệp kiểm thử
```text
tests/
├── agent_loop_test.cpp
├── agent_loop_test_v2.cpp
└── harness_runner_test.cpp
```
Nếu muốn đổi chương trình chính, hãy chỉnh sửa tệp `CMakeLists.txt`.
Ví dụ:
```cmake
add_executable(
    harness_runner_test
    tests/harness_runner_test.cpp
    ${CORE_SOURCES}
)
```
---
## Thư viện được sử dụng
* SQLite3
* libcurl
* nlohmann/json
* CMake
---
## Kết quả đầu ra
Các tệp kết quả sẽ được lưu tại:
```text
results/
```
Ví dụ:
```text
results/
├── object_name.txt
├── resultsEquation.txt
├── trajectory_task_math_01.json
├── trajectory_task_math_02.json
└── trajectory_task_memory_01.json
```