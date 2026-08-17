#pragma once
#include "tool.h"
#include <functional>
#include <string>
#include <memory>

using ExecutionPolicy = std::function<bool(const std::string& command,
                                           const std::string& explanation)>;

namespace ExecPolicy {
    inline ExecutionPolicy AlwaysAllow() {
        return [](const std::string&, const std::string&) { return true; };
    }
    inline ExecutionPolicy AlwaysDeny() {
        return [](const std::string&, const std::string&) { return false; };
    }
    
    // CHÍNH SÁCH MỚI: Tự động phân tích chuỗi từ chối từ khóa cấm, giúp không bị treo luồng test tự động
    inline ExecutionPolicy AutomatedBlacklist() {
        return [](const std::string& command, const std::string&) {
            if (command.find("rm -rf") != std::string::npos || 
                command.find("sudo") != std::string::npos || 
                command.find("shutdown") != std::string::npos) {
                return false; // AutoDeny các lệnh phá hoại hệ thống
            }
            return true; // AutoAllow các lệnh an toàn
        };
    }
    
    ExecutionPolicy AskUser();
}

class ExecTool : public Tool {
private:
    Environment* env_;      
    ExecutionPolicy policy_;

public:
    ExecTool(Environment* env, ExecutionPolicy policy = ExecPolicy::AutomatedBlacklist());
    
    std::string getName() const override { return "exec"; }
    std::string getDescription() const override {
        return "Thuc thi lenh shell/bash. Tham so: lenh shell (vd: 'ls -la'). "
               "Khong dung lenh tuong tac (nano, vim).";
    }
    std::string execute(const std::string& arguments) override;
};