#include "skill_loader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

SkillLoader::SkillLoader(fs::path skillsDir) : skillsDir_(std::move(skillsDir)) {}

std::string SkillLoader::toLowerAscii(std::string s) {
    // tolower() byte-theo-byte trên unsigned char là an toàn với UTF-8: các byte
    // ngoài phạm vi ASCII (vd phần mở rộng của ký tự có dấu tiếng Việt) sẽ được
    // tolower() trong locale "C" trả về nguyên trạng, không bị phá vỡ chuỗi UTF-8.
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

// Tách chuỗi thành các "từ" (token), coi mọi byte ASCII không phải chữ/số là
// dấu phân cách (space, dấu câu...), còn byte UTF-8 nhiều byte (>=0x80, dùng để
// mã hoá ký tự có dấu tiếng Việt) luôn được giữ làm 1 phần của token, KHÔNG bị
// coi là dấu phân cách. Nhờ vậy "trí nhớ" vẫn ra đúng 1 token "trí" + "nhớ" chứ
// không bị cắt vụn theo từng byte.
//
// Việc tokenize theo TỪ (thay vì so khớp substring thô) tránh được lỗi khớp
// nhầm kiểu keyword "hi" lại khớp ngay bên trong từ "ng-HI-eu".
std::vector<std::string> SkillLoader::tokenize(const std::string& text) {
    std::string lower = toLowerAscii(text);
    std::vector<std::string> tokens;
    std::string current;

    for (unsigned char c : lower) {
        bool isDelimiter = (c < 128) && !std::isalnum(c);
        if (isDelimiter) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(static_cast<char>(c));
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

// Kiểm tra xem dãy token "needle" có xuất hiện liên tiếp trong "haystack" hay
// không (so khớp từng token TOÀN VẸN, không phải substring).
static bool containsSubsequence(const std::vector<std::string>& haystack,
                                 const std::vector<std::string>& needle) {
    if (needle.empty() || haystack.size() < needle.size()) return false;
    for (size_t start = 0; start + needle.size() <= haystack.size(); ++start) {
        bool allMatch = true;
        for (size_t k = 0; k < needle.size(); ++k) {
            if (haystack[start + k] != needle[k]) {
                allMatch = false;
                break;
            }
        }
        if (allMatch) return true;
    }
    return false;
}

std::vector<std::string> SkillLoader::parseKeywordLine(const std::string& line) {
    // line dạng: "keywords: tinh, toan, calculator"
    auto colonPos = line.find(':');
    if (colonPos == std::string::npos) return {};

    std::string rest = line.substr(colonPos + 1);
    std::vector<std::string> keywords;
    std::stringstream ss(rest);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // trim khoảng trắng 2 đầu
        size_t start = token.find_first_not_of(" \t\r\n");
        size_t end = token.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        std::string trimmed = token.substr(start, end - start + 1);
        if (!trimmed.empty()) keywords.push_back(toLowerAscii(trimmed));
    }
    return keywords;
}

Skill SkillLoader::parseSkillFile(const fs::path& path) {
    Skill skill;
    skill.name = path.stem().string();
    skill.filePath = path.string();

    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string raw = buffer.str();

    // Tách thành các dòng để tìm frontmatter "---" ... "---" ở đầu file.
    std::istringstream lineStream(raw);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(lineStream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); // CRLF
        lines.push_back(line);
    }

    // Tìm dòng "---" mở đầu (bỏ qua các dòng trống đứng trước nó).
    size_t i = 0;
    while (i < lines.size() && lines[i].empty()) ++i;

    if (i < lines.size() && lines[i] == "---") {
        size_t frontmatterStart = i + 1;
        size_t frontmatterEnd = std::string::npos;
        for (size_t j = frontmatterStart; j < lines.size(); ++j) {
            if (lines[j] == "---") {
                frontmatterEnd = j;
                break;
            }
        }

        if (frontmatterEnd != std::string::npos) {
            for (size_t j = frontmatterStart; j < frontmatterEnd; ++j) {
                std::string trimmedLine = toLowerAscii(lines[j]);
                if (trimmedLine.rfind("keywords:", 0) == 0 ||
                    trimmedLine.rfind("keyword:", 0) == 0) {
                    skill.keywords = parseKeywordLine(lines[j]);
                }
            }

            // Nội dung skill = phần còn lại sau dòng "---" đóng frontmatter.
            std::ostringstream contentStream;
            for (size_t j = frontmatterEnd + 1; j < lines.size(); ++j) {
                contentStream << lines[j] << "\n";
            }
            skill.content = contentStream.str();
            return skill;
        }
    }

    // Không có frontmatter hợp lệ -> toàn bộ file là content, keywords rỗng.
    skill.content = raw;
    return skill;
}

size_t SkillLoader::loadAll() {
    skills_.clear();
    if (!fs::exists(skillsDir_) || !fs::is_directory(skillsDir_)) {
        return 0;
    }

    std::vector<fs::path> mdFiles;
    for (const auto& entry : fs::directory_iterator(skillsDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            mdFiles.push_back(entry.path());
        }
    }
    // directory_iterator không đảm bảo thứ tự -> sắp xếp để kết quả ổn định,
    // reproducible (quan trọng khi chạy benchmark/HarnessRunner sau này).
    std::sort(mdFiles.begin(), mdFiles.end());

    for (const auto& path : mdFiles) {
        skills_.push_back(parseSkillFile(path));
    }
    return skills_.size();
}

const Skill* SkillLoader::findByName(const std::string& name) const {
    for (const auto& skill : skills_) {
        if (skill.name == name) return &skill;
    }
    return nullptr;
}

std::vector<const Skill*> SkillLoader::selectSkillsForTask(const std::string& task) const {
    std::vector<std::string> taskTokens = tokenize(task);
    std::vector<const Skill*> matched;

    for (const auto& skill : skills_) {
        bool hit = false;
        for (const auto& keyword : skill.keywords) {
            std::vector<std::string> keywordTokens = tokenize(keyword);
            if (keywordTokens.empty()) continue;
            if (containsSubsequence(taskTokens, keywordTokens)) {
                hit = true;
                break;
            }
        }
        if (hit) matched.push_back(&skill);
    }
    return matched;
}

std::string SkillLoader::buildSkillPrompt(const std::string& task, const std::string& fallbackSkillName) const {
    std::vector<const Skill*> selected = selectSkillsForTask(task);

    if (selected.empty() && !fallbackSkillName.empty()) {
        if (const Skill* fallback = findByName(fallbackSkillName)) {
            selected.push_back(fallback);
        }
    }

    if (selected.empty()) return "";

    std::ostringstream out;
    for (const Skill* skill : selected) {
        out << "\n--- [SKILL: " << skill->name << "] ---\n" << skill->content << "\n";
    }
    return out.str();
}
