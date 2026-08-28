#include "agent_loop.h"
#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>
#include "../utils/SystemUtils.h"
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
        "- Khong de trong Action hoac Arguments.\n"
        "\n=== GHI FILE ===\n"
        "Khi can tao/ghi/sua noi dung file (dac biet la code C/C++/Python), BAT BUOC "
        "dung tool 'write_file' voi Arguments dang 'duong_dan | noi_dung'. Day la "
        "CACH DUY NHAT duoc phep de ghi file - KHONG CO NGOAI LE nao, du noi dung "
        "don gian hay phuc tap den dau, du chua dau nhay don/kep hay ky tu dac biet gi.\n"
        "Dung \\n de xuong dong khi can ghi nhieu dong trong noi_dung - tool tu dong "
        "chuyen thanh xuong dong that, KHONG can tu escape them bat ky ky tu nao khac.\n"
        "TUYET DOI KHONG dung 'exec' voi 'echo' hoac 'printf' de ghi file trong bat ky "
        "truong hop nao - lenh se bi tu choi ngay lap tuc va bao loi. Tool 'exec' CHI "
        "duoc dung de CHAY/BIEN DICH file da duoc tao san bang 'write_file' truoc do "
        "(vi du: g++ file.cpp -o app && ./app), khong duoc dung de tao noi dung file.";

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

Action AgentLoop::parseAction(const std::string& thought) {
    std::smatch m;

    // 1. SỬA REGEX FINAL ANSWER: Dùng [\s\S]+ để bắt toàn bộ văn bản nhiều dòng đến cuối chuỗi
    std::regex finalRe(R"(Final Answer:\s*([\s\S]+))", std::regex::icase);
    if (std::regex_search(thought, m, finalRe)) {
        std::string text = m[1].str();
        while (!text.empty() && (text.back() == '\r' || text.back() == '\n'))
            text.pop_back();
        return FinalAnswerAction{text};
    }

    // 2. SỬA REGEX ARGUMENTS: Tương tự, dùng [\s\S]+ để gom mọi dòng tham số
    std::regex actionRe(R"(Action:\s*([A-Za-z0-9_]+))");
    std::regex argsRe(R"(Arguments:\s*([\s\S]+))");

    std::string toolName, args;
    if (std::regex_search(thought, m, actionRe)) toolName = m[1].str();
    
    if (std::regex_search(thought, m, argsRe)) {
        args = m[1].str();
        while (!args.empty() &&
               (args.back() == '\r' || args.back() == '\n' || args.back() == ' '))
            args.pop_back();
    }

    if (!toolName.empty()) return ToolCallAction{toolName, args};

    // 3. Không parse được -> coi toàn bộ thought là Final Answer
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
    return tool->execute(action.arguments);// return env_->runTool(tool,action)
}

// ─── doObserve ───────────────────────────────────────────────────────────────

std::string AgentLoop::doObserve(const std::string& toolResult) {
    return "Observation: " + toolResult;
}

// ─── run() ─────────────────────

Trajectory AgentLoop::run(const std::string& taskId, const std::string& task, const std::string& imagePath) {
    Trajectory traj;
    traj.taskId = taskId;
    traj.task   = task;
    
    // An toàn: Kiểm tra llm_ trước khi gọi
    traj.model  = (llm_) ? llm_->getModelName() : "Unknown_Model"; 

    loopDetector_.reset();
    bool hasImage = !imagePath.empty();
    std::string binaryImage = "";

    if (hasImage){  
        binaryImage = SystemUtils::encodeImageToBase64(imagePath);
    }
    // ── Khởi tạo conversation history ────────────────────────────────────────
    std::vector<Message> messages = {
        {"system", buildSystemPrompt(task), "",false},
        {"user",   task, binaryImage, hasImage}
    };

    auto runStart = steady_clock::now();
    std::cout << "\n[AgentLoop] Bat dau thuc thi Task ID: " << taskId << "\n";

    // ── Vòng lặp ReAct chính ─────────────────────────────────────────────────
    for (int stepIdx = 0; stepIdx < maxSteps_; ++stepIdx) {
        Step step;
        step.stepId = stepIdx;
        step.tokensUsed = 0; // Đảm bảo khởi tạo giá trị an toàn

        // ── THINK ────────────────────────────────────────────────────────────
        auto t0 = steady_clock::now();
        step.thought = doThink(messages);
        step.latencyMs = duration_cast<milliseconds>(steady_clock::now() - t0).count();
        step.tokensUsed = llm_->getLastTokenUsage();
        std::cout << "\n[Step " << stepIdx << " | " << step.latencyMs << "ms]\n"
                  << step.thought << "\n";

        // Lỗi từ LLM -> dừng ngay
        if (step.thought.rfind("__ERROR__", 0) == 0) {
            std::cerr << "[AgentLoop] LLM error: " << step.thought << "\n";
            traj.steps.push_back(step);
            
            // [HOOK] Ghi nhận bước lỗi cho Harness
            if (stepHook_) stepHook_(step);
            break;
        }

        // ── PARSE ACTION ──────────────────────────────────────────────────────
        step.action = parseAction(step.thought);

        // ── Xử lý từng nhánh Action bằng std::visit + if constexpr ──
        bool shouldStop = std::visit([&](const auto& a) -> bool {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, FinalAnswerAction>) {
                traj.finalAnswer = a.text;
                traj.success     = true;
                step.toolResult  = std::nullopt; // Không gọi tool nên kết quả rỗng
                std::cout << "[AgentLoop] Final Answer: " << a.text << "\n";
                return true;
            }

            if constexpr (std::is_same_v<T, ToolCallAction>) {
                // ── LOOP DETECTION ────────────────────────────────────────────
                std::string loopReason;
                LoopStatus status = loopDetector_.check(a.toolName, a.arguments, loopReason);
                if (!loopReason.empty()) std::cout << loopReason << "\n";
                if (status == LoopStatus::Critical) {
                    traj.stoppedByLoop = true;
                    return true;
                }

                // ── ACT ───────────────────────────────────────────────────────
                std::string result = doAct(a);
                step.toolResult = result;
                std::cout << "[Tool: " << a.toolName << "] " << result << "\n";

                // ── OBSERVE: cập nhật conversation history ───────────────────
                messages.push_back({"assistant", step.thought});
                messages.push_back({"user",      doObserve(result)});
                return false;
            }

            return false;
        }, step.action);

        // =====================================================================
        // ── Record step + gọi hook (Observer pattern) ────────────────────────
        // [HOOK] Đặt đúng chuẩn: Sau khi có toolResult và trước khi break loop
        // =====================================================================
        traj.totalTokens += step.tokensUsed;
        traj.steps.push_back(step);
        
        if (stepHook_) {
            stepHook_(step); // Bắn dữ liệu về HarnessRunner
        }

        // Phá vỡ vòng lặp nếu chốt xong Final Answer hoặc gặp lỗi Loop
        if (shouldStop) break;

        if (stepIdx == maxSteps_ - 1) {
            traj.stoppedByMaxSteps = true;
            std::cout << "[AgentLoop] Dat gioi han " << maxSteps_ << " buoc. Ngung ep buoc.\n";
        }
    }

    traj.totalTimeMs = duration_cast<milliseconds>(
        steady_clock::now() - runStart).count();
    
    return traj;
}