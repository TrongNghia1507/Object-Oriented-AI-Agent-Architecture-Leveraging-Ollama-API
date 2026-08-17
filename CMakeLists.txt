cmake_minimum_required(VERSION 3.10)
project(AgentSingleTest)

# Cấu hình chuẩn C++23 cho dự án
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Khai báo các thư mục chứa file Header (.h, .hpp) để tránh lỗi No such file
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Harness
    ${CMAKE_CURRENT_SOURCE_DIR}/LLMClient
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/agent
    ${CMAKE_CURRENT_SOURCE_DIR}/environment
)

# Gom tất cả các file mã nguồn .cpp nền của hệ thống
set(CORE_SOURCES
    src/LLMClient/OllamaClient.cpp
    src/utils/base64.cpp
    src/tools/advanced_math_tool.cpp
    src/tools/calculator_tool.cpp
    src/tools/exec_tool.cpp
    src/tools/memory_tool.cpp
    src/tools/tool_registry.cpp
    src/tools/web_search_tool.cpp
    src/tools/translate_tool.cpp
    src/tools/text_to_speech.cpp
    src/tools/file_tool.cpp
    src/tools/datetime_tool.cpp
    src/tools/todolist_tool.cpp
    src/environment/environment.cpp
    src/Harness/Harness_runner.cpp
    src/agent/agent_loop.cpp
    src/agent/loop_detector.cpp
    src/agent/skill_loader.cpp
    src/utils/SystemUtils.cpp
)
add_executable(harness_runner
    main/harness_runner.cpp
    ${CORE_SOURCES}
)
target_link_libraries(harness_runner
    curl
    sqlite3
)
add_executable(interact_model
    main/interact_model.cpp
    ${CORE_SOURCES}
)
target_link_libraries(interact_model
    curl
    sqlite3
)
