#pragma once
#include "skill.h"
#include <filesystem>
#include <string>
#include <vector>

// SkillLoader (yêu cầu 3.3 đề bài):
//   - Load skill từ file .md trong thư mục skills/ (dùng std::filesystem để quét thư mục).
//   - Skill selection: chọn skill phù hợp với task dựa trên keyword matching.
//   - Build đoạn text sẵn sàng inject vào system prompt trước mỗi agent run.
//
// Định dạng 1 file skill .md — frontmatter đơn giản kiểu YAML ở đầu file:
//
//   ---
//   keywords: tính, toán, calculator, sqrt, +, -, *, /
//   ---
//   # Nội dung skill (phần này được inject vào prompt)...
//
// Nếu file không có frontmatter, skill vẫn được nạp (keywords rỗng) nhưng sẽ
// không bao giờ được chọn tự động qua keyword, trừ khi dùng làm fallback theo tên.
class SkillLoader {
private:
    std::filesystem::path skillsDir_;
    std::vector<Skill> skills_;

    static std::vector<std::string> parseKeywordLine(const std::string& line);
    static Skill parseSkillFile(const std::filesystem::path& path);
    static std::string toLowerAscii(std::string s);
    static std::vector<std::string> tokenize(const std::string& text);

public:
    explicit SkillLoader(std::filesystem::path skillsDir);

    // Quét toàn bộ *.md trong skillsDir_ (thứ tự alphabet để kết quả ổn định),
    // nạp vào skills_. Trả về số skill nạp được. Không ném exception nếu thư
    // mục rỗng/không tồn tại — trả về 0.
    size_t loadAll();

    size_t skillCount() const { return skills_.size(); }
    const std::vector<Skill>& getAllSkills() const { return skills_; }
    const Skill* findByName(const std::string& name) const;

    // Trả về các skill có >= 1 keyword xuất hiện dạng substring (không phân biệt
    // hoa/thường với ký tự ASCII) trong task.
    std::vector<const Skill*> selectSkillsForTask(const std::string& task) const;

    // Build đoạn text sẵn sàng nhét vào system prompt: nối content của các skill
    // được chọn theo keyword. Nếu không skill nào khớp và fallbackSkillName khác
    // rỗng, dùng skill có tên đó làm phương án dự phòng.
    std::string buildSkillPrompt(const std::string& task, const std::string& fallbackSkillName = "") const;
};
