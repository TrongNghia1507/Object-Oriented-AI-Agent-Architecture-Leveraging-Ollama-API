#pragma once
#include "../agent/agent_loop.h"
#include "../agent/trajectory.h"
#include "evaluator.h"
#include "../environment/environment.h" // === THÊM MỚI: Nhúng tệp môi trường vào ===
#include "../LLMClient/json.hpp"
#include <vector>
#include <memory>
#include <string>
#include "keyword_evaluator.h"    
#include "functional_evaluator.h"

class HarnessRunner {
private:
    std::vector<std::shared_ptr<Evaluator>> evaluators_;
    Environment* env_; // === THÊM MỚI: Harness nắm giữ thực thể Environment ===

public:
    HarnessRunner() = default;
    ~HarnessRunner() = default;

    // === THÊM MỚI: Hàm gán Môi trường thực thi cho bộ đai Harness ===
    void setEnvironment(Environment* env) {
        env_ = env;
    }

    void loadTaskFromJson(const std::string& filename, AgentLoop& agent) ; // void HarnessRunner::loadTaskFromJson, xoá cái trc để test tương tác prompt thôi, test xong sửa lại

    void addEvaluator(std::shared_ptr<Evaluator> evaluator);
    void clearEvaluators() { evaluators_.clear(); }  
    bool runTask(const std::string& taskId, const std::string& task, AgentLoop& agent);
};