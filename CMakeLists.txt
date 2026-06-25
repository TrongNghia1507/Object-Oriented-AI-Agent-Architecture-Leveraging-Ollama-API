cmake_minimum_required(VERSION 3.10)
project(OOP_AI_Agent CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

# Nguồn Tool dùng chung (không cần Ollama)
set(TOOL_SOURCES
    tools/tool_registry.cpp
    tools/calculator_tool.cpp
    tools/advanced_math_tool.cpp
    tools/memory_tool.cpp
    tools/exec_tool.cpp
    tools/web_search_tool.cpp
)

# Nguồn SkillLoader
set(SKILL_SOURCES
    skills/skill_loader.cpp
)

# ---------------------------------------------------------------------------
# Target 1: math_tool_test — test Tool layer, không cần Ollama
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
# Target 2: skill_loader_test — test SkillLoader, không cần Ollama
# ---------------------------------------------------------------------------
add_executable(skill_loader_test
    tests/skill_loader_test.cpp
    ${SKILL_SOURCES}
)

# ---------------------------------------------------------------------------
# Target 3: llm_tool_test — test tool + Ollama thật
# ---------------------------------------------------------------------------
add_executable(llm_tool_test
    tests/test_tool_with_llm.cpp
    LLMClient/OllamaClient.cpp
    LLMClient/base64.cpp
    ${TOOL_SOURCES}
    ${SKILL_SOURCES}
)
target_link_libraries(llm_tool_test PRIVATE SQLite::SQLite3 CURL::libcurl)

# ---------------------------------------------------------------------------
# Target 4: memory_tool_test — test memory tool + Ollama
# ---------------------------------------------------------------------------
add_executable(memory_tool_test
    test_memory.cpp
    LLMClient/OllamaClient.cpp
    LLMClient/base64.cpp
    tools/memory_tool.cpp
    utils/SystemUtils.cpp
)
target_link_libraries(memory_tool_test PRIVATE SQLite::SQLite3 CURL::libcurl)

# ---------------------------------------------------------------------------
# Target 5: math_llm_test — test math tools + Ollama
# ---------------------------------------------------------------------------
add_executable(math_llm_test
    test_math.cpp
    LLMClient/OllamaClient.cpp
    LLMClient/base64.cpp
    tools/calculator_tool.cpp
    tools/advanced_math_tool.cpp
    utils/SystemUtils.cpp
)
target_link_libraries(math_llm_test PRIVATE CURL::libcurl)

# ---------------------------------------------------------------------------
# Target 6: exec_tool_test — test ExecTool + Ollama
# ---------------------------------------------------------------------------
add_executable(exec_tool_test
    test.cpp
    LLMClient/OllamaClient.cpp
    LLMClient/base64.cpp
    tools/exec_tool.cpp
    utils/SystemUtils.cpp
)
target_link_libraries(exec_tool_test PRIVATE CURL::libcurl)

# ---------------------------------------------------------------------------
# Target 7: web_search_test — test WebSearchTool + Ollama
# ---------------------------------------------------------------------------
add_executable(web_search_test
    test_web_search.cpp
    LLMClient/OllamaClient.cpp
    LLMClient/base64.cpp
    tools/web_search_tool.cpp
    utils/SystemUtils.cpp
)
target_link_libraries(web_search_test PRIVATE CURL::libcurl)

# Các test với LLM phải chạy từ thư mục gốc dự án (không phải từ build/)
# vì đọc skills/*.md bằng đường dẫn tương đối.
