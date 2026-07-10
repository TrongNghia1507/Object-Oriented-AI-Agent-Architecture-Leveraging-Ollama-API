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
)

# Gom tất cả các file mã nguồn .cpp nền của hệ thống
set(CORE_SOURCES
    LLMClient/OllamaClient.cpp 
    LLMClient/base64.cpp
    tools/advanced_math_tool.cpp
    tools/calculator_tool.cpp
    tools/exec_tool.cpp
    tools/memory_tool.cpp
    tools/tool_registry.cpp
    tools/web_search_tool.cpp
    Harness/environment.cpp
    Harness/Harness_runner.cpp
    agent/agent_loop.cpp
    agent/loop_detector.cpp
    agent/skill_loader.cpp
    # Thêm agent_loop.cpp vào đây nếu class AgentLoop nằm trong file .cpp riêng
)

# Tạo file thực thi từ file test.cpp và các file nền
# LƯU Ý: Nếu file test.cpp nằm trong thư mục tests/, hãy sửa thành tests/test.cpp
add_executable(run_test 
    tests/agent_loop_test.cpp
    ${CORE_SOURCES}
)

# Mở comment dòng dưới nếu OllamaClient hoặc WebSearchTool sử dụng libcurl để gọi API
target_link_libraries(run_test curl)
target_link_libraries(run_test sqlite3 curl)