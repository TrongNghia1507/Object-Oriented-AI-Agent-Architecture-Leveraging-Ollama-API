# AgentAI
Hệ thống gồm 5 tầng độc lập nhưng phối hợp thành một pipeline hoàn chỉnh:

* **LLMClient** — giao tiếp với LLM (text, multi-turn, multimodal ảnh).
* **Tool Registry** — đăng ký, tra cứu và áp policy cho các tool tại runtime.
* **Skill System** — nạp và chọn skill (`.md`) theo từ khóa, inject vào system prompt.
* **AgentLoop** — vòng lặp ReAct (Think → Act → Observe) kèm loop detection.
* **Harness & Evaluator** — chạy benchmark hàng loạt, chấm điểm, xuất trajectory JSON.

---

## Cấu trúc thư mục

```text
AgentAI-main/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── agent/            # AgentLoop, LoopDetector, SkillLoader, Step, Trajectory
│   ├── LLMClient/         # LLMClient (abstract) + OllamaClient, json.hpp
│   ├── tools/             # Tool (abstract) + 10 tool cụ thể, ToolRegistry
│   ├── environment/       # Environment (abstract) + Native/SandboxEnvironment
│   ├── Harness/           # HarnessRunner, Evaluator (Keyword/Functional), message_queue
│   ├── utils/             # base64, SystemUtils
│   └── model.env          # API key & cấu hình (KHÔNG commit — xem mục Bảo mật)
├── skills/                # Các file skill .md (planner cho từng nhóm tool)
├── benchmark/
│   └── tasks.json         # Tập 10 task benchmark
├── benchmark_summary/     # Kết quả chạy: trajectory_*.json + benchmark_summary.json
├── docs/                  # Class/Sequence/Component diagram (Mermaid)
└── main/                  # interact_model.cpp (build mặc định), harness_runner.cpp, multi_agent.cpp
```

---

## Các tool đã triển khai

| Tool | File | Mô tả |
|---|---|---|
| `exec` | `exec_tool.cpp` | Chạy lệnh shell, trả stdout/stderr |
| `read_file` / `write_file` | `file_tool.cpp` | Đọc/ghi file qua `Environment` |
| `web_search` | `web_search_tool.cpp` | Tìm kiếm qua SearXNG/DuckDuckGo |
| `memory_save` / `memory_search` | `memory_tool.cpp` | Lưu & truy vấn memory qua SQLite, cosine similarity |
| `calculator` | `calculator_tool.cpp` | Tính biểu thức số học |
| `advanced_math` | `advanced_math_tool.cpp` | Tính toán nâng cao (dùng `exprtk`) |
| `datetime` | `datetime_tool.cpp` | Lấy ngày giờ theo timezone (`std::chrono::zoned_time`) |
| `todolist` | `todolist_tool.cpp` | Quản lý danh sách công việc |
| `translate` | `translate_tool.cpp` | Dịch văn bản |
| `text_to_speech` | `text_to_speech.cpp` | Chuyển văn bản thành giọng nói |

Chính sách tool (allow/deny theo tên) được quản lý qua `ToolRegistry::banTool()` / `unbanTool()`.

---

## Yêu cầu hệ thống

* CMake ≥ 3.10
* Trình biên dịch hỗ trợ **C++23** đầy đủ, bao gồm `<expected>` và `<print>`:
  * **GCC ≥ 14** 
  * Hoặc Clang ≥ 17 / MSVC tương ứng
* libcurl (dev headers)
* SQLite3 (dev headers)
* Git

## Cài đặt thư viện

### Ubuntu / GitHub Codespaces

```bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libsqlite3-dev

# Nếu compiler mặc định là GCC 13, cài thêm GCC 14:
sudo apt install -y g++-14 gcc-14
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

Mở file `src/model.env` và điền API key:

```env
OPENROUTER_API_KEY=your_api_key
TTS_API_KEY=your_api_key
MEMORY_TOP_K=5
MEMORY_THRESHOLD=0.55
MEMORY_SIM_WEIGHT=0.8
MEMORY_RECENCY_WEIGHT=0.2
MEMORY_EMBEDDING_MODEL=nomic-embed-text
```

---

## Biên dịch dự án

```bash
cd AgentAI-main
mkdir build && cd build

# Nếu compiler mặc định đã là GCC 14 trở lên:
cmake ..

# Nếu cần chỉ định rõ compiler C++23:
cmake .. -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_C_COMPILER=gcc-14

cmake --build . -j
```

## Chạy chương trình

```bash
./interact_model
```

### Chạy batch benchmark qua HarnessRunner (10 task, ghi trajectory JSON)

```bash
./harness_runner
```

### Chạy multi-agent song song 

```bash
./multi_agent
```

## Skill system

Skill được viết dưới dạng file Markdown trong `skills/`, có frontmatter khai báo từ khóa để `SkillLoader` chọn skill phù hợp với task và inject vào system prompt:

```markdown
---
keywords: tính, toán, calculator, sqrt, +, -, *, /
---
# Nội dung hướng dẫn agent...
```

Danh sách skill hiện có: `datetime_planner`, `exec_planner`, `file_planner`, `general_assistant`, `math_planner`, `memory_planner`, `text_to_speech_planner`, `todolist_planner`, `translate_planner`, `web_search_planner`.

---

## Benchmark & Evaluation

* `benchmark/tasks.json` — 10 task, phân bố 4 đơn giản / 4 trung bình / 2 khó, mỗi task khai báo `eval_type` (`keyword` hoặc `functional`).
* `KeywordEvaluator` — chấm dựa trên số từ khóa khớp trong câu trả lời cuối.
* `FunctionalEvaluator` — chạy `eval_script` qua `Environment::executeCommand()`, PASS nếu output chứa `"PASS"`.
* Kết quả tổng hợp (success rate, chi tiết từng task) được ghi ra `benchmark_summary/benchmark_summary.json`.

---

## Design patterns đã áp dụng

| Pattern | Vị trí |
|---|---|
| **Template Method** | `AgentLoop::run()` định nghĩa khung ReAct cố định; `doThink()/doAct()/doObserve()` là virtual để override |
| **Observer / Hook** | `HarnessRunner` inject `stepHook_` (`std::function<void(const Step&)>`) vào `AgentLoop` — AgentLoop không biết Harness tồn tại |
| **Strategy** | `Evaluator` (abstract) với `KeywordEvaluator`, `FunctionalEvaluator` dùng chung `evaluate()` |
| **Registry / Factory** | `ToolRegistry` đăng ký và tra cứu `Tool` theo tên tại runtime |

## Kỹ thuật C++ đã dùng

* **C++17**: `std::variant`, `std::optional`, `std::filesystem`, `structured bindings`, `std::visit` + `if constexpr`, `smart pointers`, `pure virtual interfaces`, `lambda`, `range-based for`.
* **C++20**: `std::chrono::zoned_time`, `std::format`.
* **C++23**: `std::expected` / `std::unexpected` (LLMClient), `std::println` (Harness_runner.cpp).
* **C++26**: `placeholder`.

---

## Thư viện bên thứ ba

* [nlohmann/json](https://github.com/nlohmann/json) — parse/serialize JSON
* [ExprTk](https://github.com/ArashPartow/exprtk) — parse biểu thức toán học
* libcurl — HTTP client cho LLMClient, web_search, translate, TTS
* SQLite3 — lưu trữ memory


## Tài liệu thiết kế

Xem `docs/` để biết Class Diagram, Sequence Diagram (agent run & batch evaluation), và Component Diagram dạng Mermaid.