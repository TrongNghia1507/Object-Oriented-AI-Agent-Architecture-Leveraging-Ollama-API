#pragma once
#include "step.h"
#include "trajectory.h"
#include "loop_detector.h"
#include "../LLMClient/LLMClient.h"
#include "../tools/tool_registry.h"
#include "../skills/skill_loader.h"

#include <functional>
#include <string>
#include <vector>

// ─── AgentLoop — vòng lặp ReAct chính (yêu cầu 3.4) ─────────────────────────
//
// DESIGN PATTERN 1: Template Method (IV.4.2 bắt buộc)
//   run() định nghĩa bộ khung cố định: Think → Act → Observe → lặp.
//   Các bước con doThink() / parseAction() / doAct() / doObserve() là virtual
//   → subclass override để mock (unit test) hoặc tuỳ chỉnh hành vi.
//   Caller KHÔNG được override run().
//
// DESIGN PATTERN 2: Observer / Hook (IV.4.2 bắt buộc)
//   HarnessRunner inject step_hook qua setStepHook() — 1 std::function<void(const Step&)>.
//   AgentLoop gọi hook sau mỗi Step mà KHÔNG biết HarnessRunner là ai.
//   Thoả nguyên tắc "AgentLoop KHÔNG biết Harness tồn tại" (bảng III đề bài).
//
// KỸ THUẬT C++17 SỬ DỤNG:
//   std::variant<ToolCallAction, FinalAnswerAction>  → Action type
//   std::optional<string>                            → tool result trong Step
//   std::function<void(const Step&)>                 → step hook
//   std::visit + if constexpr                        → xử lý variant không cần dynamic_cast
//   structured bindings (trong ToolRegistry loop)    → auto& [k, v]

class AgentLoop {
protected:
    LLMClient*   llm_;
    ToolRegistry* registry_;
    SkillLoader*  skillLoader_;  // nullable — nếu nullptr không inject skill
    int           maxSteps_;
    LoopDetector  loopDetector_;

    // Observer hook — nullptr khi không có HarnessRunner
    std::function<void(const Step&)> stepHook_;

    // ── Các bước có thể override (Template Method) ────────────────────────────

    // Think: gửi conversation history lên LLM, nhận raw thought.
    // Override để inject mock response trong unit test.
    virtual std::string doThink(const std::vector<Message>& messages);

    // Parse: tách raw thought → Action (ToolCallAction | FinalAnswerAction).
    // Override để hỗ trợ định dạng khác (vd: JSON tool call).
    virtual Action parseAction(const std::string& thought);

    // Act: tra ToolRegistry, gọi tool, trả về kết quả.
    // Override để mock tool execution.
    virtual std::string doAct(const ToolCallAction& action);

    // Observe: format kết quả tool thành turn "user" trong conversation history.
    // Override để thêm metadata (timestamp, token count, v.v.).
    virtual std::string doObserve(const std::string& toolResult);

    // Build system prompt (tool descriptions + skill content phù hợp với task).
    virtual std::string buildSystemPrompt(const std::string& task);

public:
    AgentLoop(LLMClient*    llm,
              ToolRegistry* registry,
              SkillLoader*  skillLoader = nullptr,
              int           maxSteps    = 10);

    virtual ~AgentLoop() = default;

    // ── Cấu hình ──────────────────────────────────────────────────────────────

    // Inject step hook từ HarnessRunner (Observer pattern).
    // AgentLoop không biết ai gọi — chỉ biết mình phải gọi hook sau mỗi step.
    void setStepHook(std::function<void(const Step&)> hook) {
        stepHook_ = std::move(hook);
    }

    void setMaxSteps(int n) { maxSteps_ = n; }

    void setLoopThresholds(int warning, int critical) {
        loopDetector_ = LoopDetector(warning, critical);
    }

    // ── Template Method — KHÔNG override ──────────────────────────────────────
    // taskId: định danh task (dùng đặt tên file trajectory sau này).
    // task: câu lệnh người dùng / HarnessRunner truyền vào.
    Trajectory run(const std::string& taskId, const std::string& task);
};
