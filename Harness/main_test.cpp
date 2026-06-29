#include "../agent/agent_loop.h"
#include "Harness_runner.h"
#include "keyword_evaluator.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "==== KIEM THU DOC LAP HARNESS RUNNER ====\n\n";

    // 1. Khởi tạo AgentLoop giả lập (Truyền nullptr vì ta chưa cần LLM thật)
    AgentLoop mock_agent(nullptr, nullptr, nullptr);

    // 2. Khởi tạo Harness
    HarnessRunner harness;

    // 3. Thêm bộ chấm điểm từ khóa
    // Ta kỳ vọng AI giả lập sẽ trả lời có chữ "Hà Nội" và "nắng"
    std::vector<std::string> target_words = {"Hà Nội", "nắng"};
    auto kw_eval = std::make_shared<KeywordEvaluator>(target_words, 10); // 10 điểm 1 từ
    harness.addEvaluator(kw_eval);

    // 4. Kích hoạt bài test
    std::string task_id = "test_001";
    std::string prompt = "Thoi tiet hom nay the nao?";
    
    // Harness sẽ tiêm hook vào mock_agent và thu thập dữ liệu
    harness.runTask(task_id, prompt, mock_agent);

    std::cout << "\n==== KET THUC KIEM THU ====\n";
    return 0;
}