#pragma once
#include "tool.h"
#include <functional>
#include <string>
#include <array>
#include <memory>

// Chính sách phê duyệt lệnh shell — Strategy pattern (IV.4.2).
// AgentLoop/HarnessRunner inject policy phù hợp với ngữ cảnh:
//   - Benchmark/test tự động: AlwaysAllow hoặc AlwaysDeny
//   - Demo live: AskUser (dừng để người dùng xác nhận)
// ExecTool KHÔNG tự quyết định — nó delegate cho policy.
//
// std::function (C++17): type-erased callable, dễ inject lambda từ ngoài.
using ExecutionPolicy = std::function<bool(const std::string& command,
                                           const std::string& explanation)>;

// Các policy dựng sẵn
namespace ExecPolicy {
    // Tự động chấp thuận mọi lệnh (dùng khi benchmark / CI)
    inline ExecutionPolicy AlwaysAllow() {
        return [](const std::string&, const std::string&) { return true; };
    }
    // Từ chối mọi lệnh (dùng khi môi trường Sandbox)
    inline ExecutionPolicy AlwaysDeny() {
        return [](const std::string&, const std::string&) { return false; };
    }
    // Hỏi người dùng qua stdin (chỉ dùng khi demo live, KHÔNG dùng trong benchmark)
    ExecutionPolicy AskUser();
}

class ExecTool : public Tool {
private:
    ExecutionPolicy policy_;
    Environment* env_;
    std::string runSystemCommand(const std::string& cmd);

public:
    // Constructor nhận policy — mặc định là AskUser (an toàn nhất cho môi trường tương tác).
    explicit ExecTool(ExecutionPolicy policy = ExecPolicy::AskUser());


    explicit ExecTool(Environment* env) : env_(env) {}

    
    std::string getName() const override { return "exec"; }
    std::string getDescription() const override {
        return "Thuc thi lenh shell/bash. Tham so: lenh shell (vd: 'ls -la results/'). "
               "Khong dung lenh tuong tac (nano, vim, top). "
               "Moi lan goi la phien terminal moi (cd khong co tac dung tich luy).";
    }
    std::string execute(const std::string& arguments) override;
};
