// skill_loader_test.cpp
//
// Test độc lập cho SkillLoader: không cần Ollama, không cần network.
// Kiểm tra: (1) load đúng số skill từ thư mục skills/, (2) parse đúng
// frontmatter keywords, (3) keyword matching chọn đúng skill cho từng task mẫu.
//
// Chạy từ thư mục gốc dự án: ./build/skill_loader_test

#include "../agent/skill_loader.h"
#include <iostream>

static void printSelection(const SkillLoader& loader, const std::string& task) {
    auto matched = loader.selectSkillsForTask(task);
    std::cout << "Task: \"" << task << "\"\n  -> Khớp " << matched.size() << " skill: ";
    if (matched.empty()) {
        std::cout << "(không có)";
    } else {
        for (size_t i = 0; i < matched.size(); ++i) {
            std::cout << matched[i]->name;
            if (i + 1 < matched.size()) std::cout << ", ";
        }
    }
    std::cout << "\n\n";
}

int main() {
    SkillLoader loader("skills");
    size_t count = loader.loadAll();

    std::cout << "=== Da nap " << count << " skill tu thu muc skills/ ===\n";
    for (const auto& skill : loader.getAllSkills()) {
        std::cout << "- " << skill.name << " (" << skill.keywords.size() << " keyword, "
                  << skill.content.size() << " ky tu noi dung)\n";
    }
    std::cout << "\n";

    if (count < 3) {
        std::cerr << "[CANH BAO] De bai yeu cau toi thieu 3 skill file, hien chi co " << count << ".\n\n";
    }

    std::cout << "=== Test keyword matching ===\n\n";
    printSelection(loader, "Tinh (15 + 5) * 17");
    printSelection(loader, "sqrt(144) + 2^3 bang bao nhieu");
    printSelection(loader, "Tim lai ket qua truoc do trong tri nho");
    printSelection(loader, "Xin chao, ban la ai vay?");
    printSelection(loader, "Hom nay troi dep qua"); // không khớp skill nào

    std::cout << "=== Test buildSkillPrompt (co fallback = general_assistant) ===\n";
    std::string prompt = loader.buildSkillPrompt("Hom nay troi dep qua", "general_assistant");
    std::cout << (prompt.empty() ? "(rong)" : prompt) << "\n";

    return 0;
}
