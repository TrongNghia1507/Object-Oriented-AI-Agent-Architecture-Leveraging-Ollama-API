cmake_minimum_required(VERSION 3.10)
project(AgentAI CXX)

# Bật chuẩn C++26 hoặc fallback về C++2c / C++23
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bổ sung cờ biên dịch trực tiếp cho GCC / Clang để ép trình biên dịch nhận C++26 / C++2c
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-std=c++26) 
    # Nếu GCC 14 chưa nhận -std=c++26 thì có thể dùng cờ alias: -std=c++2c
endif()

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
add_executable(run_harness
    main/run_harness.cpp
    ${CORE_SOURCES}
)
target_link_libraries(run_harness
    curl
    sqlite3
)
add_executable(multi_agent
    main/multi_agent.cpp
    ${CORE_SOURCES}
)
target_link_libraries(multi_agent
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
