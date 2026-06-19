cmake_minimum_required(VERSION 3.10)
project(OOP_AI_Agent_Test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Khai báo các file mã nguồn tham gia biên dịch chương trình kiểm thử độc lập
add_executable(math_tool_test 
    main_test.cpp
    tools/calculator_tool.cpp
    tools/advanced_math_tool.cpp
)

# Chú ý: Đảm bảo thư mục hiện tại của bạn chứa file interface trừu tượng tool.h gốc của đồ án