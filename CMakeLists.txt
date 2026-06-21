cmake_minimum_required(VERSION 3.10)
project(OOP_AI_Agent CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

# ---------------------------------------------------------------------------
# Target 1: math_tool_test
# Test độc lập cho calculator / advanced_math / memory tool (không cần Ollama).
# ---------------------------------------------------------------------------
add_executable(math_tool_test
    main_test.cpp
    tools/calculator_tool.cpp
    tools/advanced_math_tool.cpp
    tools/memory_tool.cpp
    tools/tool_registry.cpp
)
target_link_libraries(math_tool_test PRIVATE SQLite::SQLite3)

# ---------------------------------------------------------------------------
# Target 2: llm_tool_test
# Test tool kết hợp với model thật qua Ollama (xem README.md mục "Test tool với model").
# ---------------------------------------------------------------------------
add_executable(llm_tool_test
    tests/test_tool_with_llm.cpp
    LLMClient/OllamaClient.cpp
    LLMClient/base64.cpp
    tools/tool_registry.cpp
    tools/calculator_tool.cpp
    tools/advanced_math_tool.cpp
    tools/memory_tool.cpp
    skills/skill_loader.cpp
)
target_link_libraries(llm_tool_test PRIVATE SQLite::SQLite3 CURL::libcurl)

# ---------------------------------------------------------------------------
# Target 3: skill_loader_test
# Test độc lập cho SkillLoader (parse frontmatter + keyword matching),
# không cần Ollama, không cần network.
# ---------------------------------------------------------------------------
add_executable(skill_loader_test
    tests/skill_loader_test.cpp
    skills/skill_loader.cpp
)

# llm_tool_test đọc skills/*.md bằng đường dẫn tương đối "skills/",
# nên phải chạy từ thư mục gốc dự án (nơi chứa thư mục skills/), không phải từ build/.
