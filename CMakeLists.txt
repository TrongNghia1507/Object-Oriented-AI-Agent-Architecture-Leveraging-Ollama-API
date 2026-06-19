# cmake_minimum_required(VERSION 3.10)
# project(OOP_AI_Agent_Test CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Khai báo các file mã nguồn tham gia biên dịch chương trình kiểm thử độc lập
# add_executable(math_tool_test 
#     main_test.cpp
#     tools/calculator_tool.cpp
#     tools/advanced_math_tool.cpp
#     tools/memory_tool.cpp
#     tools/memory_tool.h
# )
# Tìm kiếm thư viện SQLite3 trong hệ thống
# find_package(SQLite3 REQUIRED)

# Liên kết thư viện SQLite3 vào executable của bạn (giả sử target của bạn tên là ai_agent)
# target_link_libraries(ai_agent PRIVATE SQLite::SQLite3)

# Chú ý: Đảm bảo thư mục hiện tại của bạn chứa file interface trừu tượng tool.h gốc của đồ án

cmake_minimum_required(VERSION 3.10)
project(OOP_AI_Agent_Test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Khai báo các file mã nguồn tham gia biên dịch chương trình kiểm thử độc lập
# Lưu ý: Thường chúng ta không cần đưa file .h vào add_executable, nhưng để cũng không gây lỗi.
add_executable(math_tool_test 
    main_test.cpp
    tools/calculator_tool.cpp
    tools/advanced_math_tool.cpp
    tools/memory_tool.cpp
)

# Tìm kiếm thư viện SQLite3 trong hệ thống
find_package(SQLite3 REQUIRED)

# SỬA LỖI Ở ĐÂY: Đổi 'ai_agent' thành 'math_tool_test' để khớp với tên ở trên
target_link_libraries(math_tool_test PRIVATE SQLite::SQLite3)