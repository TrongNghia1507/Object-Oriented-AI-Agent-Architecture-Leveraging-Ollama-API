#pragma once
#include "step.h"
#include "../LLMClient/json.hpp"
#include <string>
#include <vector>

// Trajectory: toàn bộ lịch sử 1 lần agent run.
// Format toJson() khớp với mục 7.1 trong đề bài.
// AgentLoop ghi vào → HarnessRunner đọc và lưu file trajectory_{taskId}.json.
struct Trajectory {
    std::string taskId;
    std::string task;
    std::string model;
    std::string finalAnswer;
    bool success           = false;
    bool stoppedByLoop     = false;
    bool stoppedByMaxSteps = false;
    long long totalTimeMs  = 0;
    int totalTokens        = 0;
    std::vector<Step> steps;

    // Xuất JSON theo format đề bài mục 7.1.
    // Dùng std::visit + if constexpr (C++17) để xử lý std::variant<Action>.
    nlohmann::json toJson() const {
        using json = nlohmann::json;
        json stepsArr = json::array();

        for (const auto& step : steps) {
            json actionJson;
            std::visit([&](const auto& a) {
                using T = std::decay_t<decltype(a)>;
                if constexpr (std::is_same_v<T, ToolCallAction>) {
                    actionJson = {
                        {"type", "tool_call"},
                        {"tool", a.toolName},
                        {"args", a.arguments}
                    };
                } else if constexpr (std::is_same_v<T, FinalAnswerAction>) {
                    actionJson = {
                        {"type", "final_answer"},
                        {"text", a.text}
                    };
                }
            }, step.action);

            json stepJson = {
                {"step_id",     step.stepId},
                {"thought",     step.thought},
                {"action",      actionJson},
                {"action_log",  step.actionLog},  // Dữ liệu mới đã thêm
                {"observation", step.observation},
                {"tokens_used", step.tokensUsed},
                {"latency_ms",  step.latencyMs}
            };
            if (step.toolResult.has_value()) {
                stepJson["tool_result"] = *step.toolResult;
            }
            stepsArr.push_back(std::move(stepJson));
        }

        return {
            {"task_id",              taskId},
            {"task",                 task},
            {"model",                model},
            {"success",              success},
            {"stopped_by_loop",      stoppedByLoop},
            {"stopped_by_max_steps", stoppedByMaxSteps},
            {"final_answer",         finalAnswer},
            {"total_tokens",         totalTokens},
            {"total_time_ms",        totalTimeMs},
            {"steps",                stepsArr}
        };
    }
};
