#pragma once
#include <string>
#include <vector>

// Đại diện cho 1 skill đã được nạp từ 1 file .md trong thư mục skills/.
// Đây là kiểu dữ liệu thuần (data class), không có hành vi — SkillLoader chịu
// trách nhiệm tạo và chọn các Skill này.
struct Skill {
    std::string name;                  // tên skill = tên file không gồm .md (vd "math_planner")
    std::string filePath;               // đường dẫn file gốc, phục vụ debug/log
    std::vector<std::string> keywords;  // từ khóa để chọn skill, khai trong frontmatter
    std::string content;                // nội dung Markdown (đã bỏ frontmatter) để inject vào prompt
};
