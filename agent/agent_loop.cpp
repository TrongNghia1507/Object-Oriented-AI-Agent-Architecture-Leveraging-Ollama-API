#include "agent_loop.h"
#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>

using namespace std::chrono;

// ─── Constructor ──────────────────────────────────────────────────────────────

AgentLoop::AgentLoop(LLMClient* llm,
                     ToolRegistry* registry,
                     SkillLoader* skillLoader,
                     int maxSteps)
    : llm_(llm)
    , registry_(registry)
    , skillLoader_(skillLoader)
    , maxSteps_(maxSteps)
    , loopDetector_(2, 3) {}

// ─── buildSystemPrompt ────────────────────────────────────────────────────────

std::string AgentLoop::buildSystemPrompt(const std::string& task) {
    // Phần 1: mô tả tool
    std::string prompt =
        "Ban la mot AI Agent. Nhiem vu cua ban la giai quyet yeu cau cua nguoi dung "
        "bang cach su dung cac tool duoc cung cap.\n\n"
        + registry_->generateToolsPrompt();

    // Phần 2: inject skill phù hợp với task (nếu có SkillLoader)
    if (skillLoader_) {
        std::string skill = skillLoader_->buildSkillPrompt(task, "general_assistant");
        if (!skill.empty()) prompt += "\n" + skill;
    }

    // Phần 3: quy tắc trả lời — nhất quán với format trong tests/test_tool_with_llm.cpp
    prompt +=
        "\n=== QUY TAC TRA LOI ===\n"
        "Neu can dung tool, tra loi CHINH XAC 3 dong sau (khong them gi khac):\n"
        "Thought: <ly do chon tool>\n"
        "Action: <ten tool>\n"
        "Arguments: <tham so truyen cho tool>\n\n"
        "Neu da co ket qua cuoi cung (khong can goi them tool), tra loi:\n"
        "Final Answer: <cau tra loi day du bang tieng Viet>\n\n"
        "TUYET DOI KHONG:\n"
        "- Khong them preamble truoc 3 dong tren.\n"
        "- Khong goi them tool sau khi da viet Final Answer.\n"
        "- Khong de trong Action hoac Arguments.\n";

    return prompt;
}

// ─── doThink ─────────────────────────────────────────────────────────────────

std::string AgentLoop::doThink(const std::vector<Message>& messages) {
    auto result = llm_->sendMessages(messages);
    if (!result.has_value()) {
        switch (result.error()) {
            case LLMError::Timeout:
                return "__ERROR__:timeout";
            case LLMError::ConnectionDenied:
                return "__ERROR__:connection_denied";
            case LLMError::MalformedJson:
                return "__ERROR__:malformed_json";
            default:
                return "__ERROR__:unknown";
        }
    }
    return *result;
}

// ─── parseAction ─────────────────────────────────────────────────────────────

Action AgentLoop::parseAction(const std::string& thought) {
    std::smatch m;

    // 1. Kiểm tra "Final Answer:" trước (ưu tiên cao hơn)
    std::regex finalRe(R"(Final Answer:\s*(.+))", std::regex::icase);
    if (std::regex_search(thought, m, finalRe)) {
        std::string text = m[1].str();
        while (!text.empty() && (text.back() == '\r' || text.back() == '\n'))
            text.pop_back();
        return FinalAnswerAction{text};
    }

    // 2. Parse "Action: <tool>" và "Arguments: <args>"
    std::regex actionRe(R"(Action:\s*([A-Za-z0-9_]+))");
    std::regex argsRe(R"(Arguments:\s*(.+))");

    std::string toolName, args;
    if (std::regex_search(thought, m, actionRe)) toolName = m[1].str();
    if (std::regex_search(thought, m, argsRe)) {
        args = m[1].str();
        while (!args.empty() &&
               (args.back() == '\r' || args.back() == '\n' || args.back() == ' '))
            args.pop_back();
    }

    if (!toolName.empty()) return ToolCallAction{toolName, args};

    // 3. Không parse được → coi toàn bộ thought là Final Answer
    return FinalAnswerAction{thought};
}

// ─── doAct ───────────────────────────────────────────────────────────────────

std::string AgentLoop::doAct(const ToolCallAction& action) {
    Tool* tool = registry_->getTool(action.toolName);
    if (!tool) {
        // Trả về thông báo lỗi để model tự điều chỉnh ở bước sau
        std::string names;
        for (const auto& n : registry_->getAllToolNames()) names += n + ", ";
        return "[LOI] Tool '" + action.toolName + "' khong ton tai. "
               "Cac tool co san: " + (names.empty() ? "(khong co)" : names.substr(0, names.size()-2));
    }
    return tool->execute(action.arguments);
}

// ─── doObserve ───────────────────────────────────────────────────────────────

std::string AgentLoop::doObserve(const std::string& toolResult) {
    return "Observation: " + toolResult;
}

// ─── run() — Template Method ──────────────────────────────────────────────────

//Tạm off để t test Harness runner
// Trajectory AgentLoop::run(const std::string& taskId, const std::string& task) {
//     Trajectory traj;
//     traj.taskId = taskId;
//     traj.task   = task;
//     traj.model  = llm_->getModelName();

//     loopDetector_.reset();

//     // ── Khởi tạo conversation history ────────────────────────────────────────
//     std::vector<Message> messages = {
//         {"system", buildSystemPrompt(task)},
//         {"user",   task}
//     };

//     auto runStart = steady_clock::now();

//     // ── Vòng lặp ReAct chính ─────────────────────────────────────────────────
//     for (int stepIdx = 0; stepIdx < maxSteps_; ++stepIdx) {
//         Step step;
//         step.stepId = stepIdx;

//         // ── THINK ────────────────────────────────────────────────────────────
//         auto t0 = steady_clock::now();
//         step.thought = doThink(messages);
//         step.latencyMs = duration_cast<milliseconds>(steady_clock::now() - t0).count();

//         std::cout << "\n[Step " << stepIdx << " | " << step.latencyMs << "ms]\n"
//                   << step.thought << "\n";

//         // Lỗi từ LLM → dừng ngay
//         if (step.thought.rfind("__ERROR__", 0) == 0) {
//             std::cerr << "[AgentLoop] LLM error: " << step.thought << "\n";
//             traj.steps.push_back(step);
//             if (stepHook_) stepHook_(step);
//             break;
//         }

//         // ── PARSE ACTION ──────────────────────────────────────────────────────
//         step.action = parseAction(step.thought);

//         // ── Xử lý từng nhánh Action bằng std::visit + if constexpr (C++17) ──
//         bool shouldStop = std::visit([&](const auto& a) -> bool {
//             using T = std::decay_t<decltype(a)>;

//             if constexpr (std::is_same_v<T, FinalAnswerAction>) {
//                 traj.finalAnswer = a.text;
//                 traj.success     = true;
//                 step.toolResult  = std::nullopt;
//                 std::cout << "[AgentLoop] Final Answer: " << a.text << "\n";
//                 return true;
//             }

//             if constexpr (std::is_same_v<T, ToolCallAction>) {
//                 // ── LOOP DETECTION ────────────────────────────────────────────
//                 std::string loopReason;
//                 LoopStatus status = loopDetector_.check(a.toolName, a.arguments, loopReason);
//                 if (!loopReason.empty()) std::cout << loopReason << "\n";
//                 if (status == LoopStatus::Critical) {
//                     traj.stoppedByLoop = true;
//                     return true;
//                 }

//                 // ── ACT ───────────────────────────────────────────────────────
//                 std::string result = doAct(a);
//                 step.toolResult = result;
//                 std::cout << "[Tool: " << a.toolName << "] " << result << "\n";

//                 // ── OBSERVE: cập nhật conversation history ───────────────────
//                 messages.push_back({"assistant", step.thought});
//                 messages.push_back({"user",      doObserve(result)});
//                 return false;
//             }

//             return false;
//         }, step.action);

//         // ── Record step + gọi hook (Observer pattern) ─────────────────────────
//         traj.totalTokens += step.tokensUsed;
//         traj.steps.push_back(step);
//         if (stepHook_) stepHook_(step);

//         if (shouldStop) break;

//         if (stepIdx == maxSteps_ - 1) {
//             traj.stoppedByMaxSteps = true;
//             std::cout << "[AgentLoop] Dat gioi han " << maxSteps_ << " buoc.\n";
//         }
//     }

//     traj.totalTimeMs = duration_cast<milliseconds>(
//         steady_clock::now() - runStart).count();
//     return traj;
// }

//MOCK TEST
Trajectory AgentLoop::run(const std::string& taskId, const std::string& task) {
    // =========================================================================
    // PHẦN 1: MOCK DATA (GIẢ LẬP ĐỂ TEST HARNESS)
    // =========================================================================
    Trajectory traj;
    traj.taskId = taskId;
    traj.task   = task;
    traj.model  = "mock-gemma4"; // Tên model giả lập

    std::cout << "\n[AgentLoop Mock] Dang chay gia lap cho task: " << task << "\n";

    // --- BƯỚC 1: GIẢ LẬP GỌI TOOL TÌM KIẾM ---
    Step step1;
    step1.stepId = 0;
    step1.thought = "Tôi cần tìm kiếm thời tiết hôm nay tại Hà Nội.";
    step1.latencyMs = 150;
    
    // Tạo cấu trúc ToolCallAction
    ToolCallAction toolAction;
    toolAction.toolName = "web_search_tool";
    toolAction.arguments = "{\"query\": \"thoi tiet Ha Noi\"}";
    step1.action = toolAction; // Gán vào biến variant
    
    // Giả lập kết quả trả về từ tool
    step1.toolResult = "Nhiệt độ tại Hà Nội hôm nay là 30 độ C, trời có nắng.";
    step1.tokensUsed = 50;

    // Ghi nhận vào Trajectory và KÍCH HOẠT HOOK cho Harness
    traj.steps.push_back(step1);
    if (stepHook_) stepHook_(step1); // Hoặc notifyHook(step1) nếu bạn đã dùng Safe Wrapper

    // --- BƯỚC 2: GIẢ LẬP ĐƯA RA CÂU TRẢ LỜI CUỐI CÙNG ---
    Step step2;
    step2.stepId = 1;
    step2.thought = "Đã có đủ dữ liệu, tôi sẽ đưa ra câu trả lời cuối cùng.";
    step2.latencyMs = 120;

    // Tạo cấu trúc FinalAnswerAction
    FinalAnswerAction finalAction;
    finalAction.text = "Thời tiết hôm nay tại Hà Nội rất đẹp và có nắng, nhiệt độ khoảng 30 độ C.";
    step2.action = finalAction; 
    step2.tokensUsed = 30;

    // Ghi nhận và báo cáo cho Harness
    traj.steps.push_back(step2);
    if (stepHook_) stepHook_(step2);

    // --- TỔNG KẾT TRAJECTORY ---
    traj.finalAnswer = finalAction.text;
    traj.success = true;
    traj.totalTokens = step1.tokensUsed + step2.tokensUsed;
    traj.totalTimeMs = step1.latencyMs + step2.latencyMs;

    return traj; // Trả về kết quả ngay lập tức để Harness xử lý


}