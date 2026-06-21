// test_tool_with_llm.cpp
//
// Demo nhỏ kiểm tra tầng Tool hoạt động đúng KHI kết hợp với LLM thật (qua Ollama),
// trước khi có AgentLoop hoàn chỉnh. Luồng chạy ở đây chính là 1 vòng ReAct viết tay:
//
//   1. Build system prompt = mô tả các tool (ToolRegistry::generateToolsPrompt())
//                              + nội dung các file skills/*.md
//   2. Gửi prompt + task của người dùng cho model (LLMClient::sendPrompt)
//   3. Parse "Action: <tool>" / "Arguments: <args>" từ câu trả lời của model
//   4. Gọi đúng Tool tương ứng trong ToolRegistry, lấy kết quả
//   5. Gửi kết quả tool quay lại cho model để model đưa câu trả lời cuối cùng
//
// Cách chạy: xem README.md mục "Test tool với model".

#include "../LLMClient/OllamaClient.h"
#include "../tools/tool_registry.h"
#include "../tools/calculator_tool.h"
#include "../tools/advanced_math_tool.h"
#include "../tools/memory_tool.h"
#include "../skills/skill_loader.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>

// Tìm dòng "Action: xxx" và "Arguments: yyy" trong câu trả lời của model.
struct ParsedAction {
    std::string toolName;
    std::string arguments;
    bool found = false;
};

static ParsedAction parseToolCall(const std::string& llmText) {
    ParsedAction result;
    std::smatch match;

    std::regex actionRe(R"(Action:\s*([A-Za-z0-9_]+))");
    std::regex argsRe(R"(Arguments:\s*(.+))");

    if (std::regex_search(llmText, match, actionRe)) {
        result.toolName = match[1].str();
        result.found = true;
    }
    if (std::regex_search(llmText, match, argsRe)) {
        std::string args = match[1].str();
        // Cắt khoảng trắng/CR thừa ở cuối dòng
        while (!args.empty() && (args.back() == '\r' || args.back() == ' ')) args.pop_back();
        result.arguments = args;
    }
    return result;
}

int main() {
    // ---- 1. Đọc cấu hình từ biến môi trường (không hardcode secret trong code!) ----
    const char* modelEnv = std::getenv("OLLAMA_MODEL");
    const char* baseUrlEnv = std::getenv("OLLAMA_BASE_URL");     // vd: http://localhost:11434
    const char* gistIdEnv = std::getenv("OLLAMA_GIST_ID");       // chỉ cần nếu dùng tunnel Pinggy
    const char* gistPatEnv = std::getenv("OLLAMA_GIST_PAT");

    if (!modelEnv) {
        std::cerr << "Thieu bien moi truong OLLAMA_MODEL. Xem README.md de biet cach set.\n";
        return 1;
    }

    std::unique_ptr<LLMClient> llm;
    if (baseUrlEnv) {
        llm = std::make_unique<OllamaClient>(modelEnv, baseUrlEnv);
        std::cout << "[Config] Dung base URL truc tiep: " << baseUrlEnv << "\n";
    } else if (gistIdEnv && gistPatEnv) {
        llm = std::make_unique<OllamaClient>(
            OllamaClient::withGistDiscovery(modelEnv, gistIdEnv, gistPatEnv));
        std::cout << "[Config] Dung che do do URL qua GitHub Gist.\n";
    } else {
        std::cerr << "Can OLLAMA_BASE_URL hoac (OLLAMA_GIST_ID + OLLAMA_GIST_PAT). Xem README.md.\n";
        return 1;
    }

    // ---- 2. Đăng ký tool ----
    ToolRegistry registry;
    registry.registerTool(std::make_unique<CalculatorTool>());
    registry.registerTool(std::make_unique<AdvancedMathTool>());

    auto dbManager = std::make_shared<DatabaseManager>();
    registry.registerTool(std::make_unique<MemorySaveTool>(dbManager));
    registry.registerTool(std::make_unique<MemorySearchTool>(dbManager));

    // ---- 3. Nạp skill (SkillLoader quét skills/, parse keyword trong frontmatter) ----
    SkillLoader skillLoader("skills");
    size_t skillCount = skillLoader.loadAll();
    std::cout << "[Config] Da nap " << skillCount << " skill tu thu muc skills/\n";

    std::string task = "Tinh (15 + 5) * 17 roi luu ket qua vao tri nho voi nhan math_result.";
    std::cout << "\n=== TASK ===\n" << task << "\n";

    // Skill selection: chỉ inject skill khớp keyword với task hiện tại
    // (fallback "general_assistant" nếu task không khớp skill chuyên biệt nào).
    std::string skillPrompt = skillLoader.buildSkillPrompt(task, "general_assistant");

    // ---- 4. Build prompt: mô tả tool + skill đã chọn + nguyên tắc trả lời theo format ReAct ----
    std::string systemPrompt =
        "Ban la mot AI agent co the dung tool de giai quyet yeu cau.\n" +
        registry.generateToolsPrompt() +
        skillPrompt +
        "\nKhi can dung tool, BAT BUOC tra loi dung 3 dong theo dinh dang sau (khong them gi khac):\n"
        "Thought: <ly do chon tool>\n"
        "Action: <ten tool, vi du calculator>\n"
        "Arguments: <tham so truyen cho tool>\n";

    // ---- 5. Vòng 1: hỏi model nên gọi tool nào ----
    auto reply1 = llm->sendPrompt(systemPrompt + "\nYeu cau: " + task);
    if (!reply1.has_value()) {
        std::cerr << "Loi goi LLM (vong 1). Kiem tra Ollama da chay/tunnel con song chua.\n";
        return 1;
    }
    std::cout << "\n=== MODEL TRA LOI (vong 1) ===\n" << *reply1 << "\n";

    ParsedAction action = parseToolCall(*reply1);
    if (!action.found) {
        std::cout << "\n[Info] Model khong goi tool nao, co the da tra loi truc tiep.\n";
        return 0;
    }

    // ---- 6. Thực thi tool ----
    Tool* tool = registry.getTool(action.toolName);
    if (!tool) {
        std::cerr << "\n[Loi] Model goi tool '" << action.toolName << "' nhung tool nay khong ton tai trong registry.\n";
        return 1;
    }
    std::string toolResult = tool->execute(action.arguments);
    std::cout << "\n=== KET QUA TOOL '" << action.toolName << "' ===\n"
              << "Arguments: " << action.arguments << "\n"
              << "Result: " << toolResult << "\n";

    // ---- 7. Vòng 2: đưa kết quả tool (Observation) lại cho model để chốt câu trả lời ----
    std::string followUp = systemPrompt +
        "\nYeu cau: " + task +
        "\nBan da goi Action: " + action.toolName + " voi Arguments: " + action.arguments +
        "\nObservation: " + toolResult +
        "\nHay dua cau tra loi cuoi cung cho nguoi dung bang tieng Viet, khong can goi them tool.";

    auto reply2 = llm->sendPrompt(followUp);
    if (!reply2.has_value()) {
        std::cerr << "Loi goi LLM (vong 2).\n";
        return 1;
    }
    std::cout << "\n=== MODEL TRA LOI CUOI CUNG (vong 2) ===\n" << *reply2 << "\n";

    return 0;
}
