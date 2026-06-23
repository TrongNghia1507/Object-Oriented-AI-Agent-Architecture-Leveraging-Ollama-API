#pragma once
#include "tool.h"
#include <iostream>
#include <string>
#include <array>
#include <memory>
class ExecTool : public Tool {
public:
    std::string execute(const json& payload) override;
    std::string getName() const override { return "exec"; };
    std::string getDescription() const {
        return "Thực thi lệnh shell/bash trên hệ thống. Trả về stdout và stderr.";
    }

private:
    // Hàm thực thi lệnh gom stdout + stderr
    std::string runSystemCommand(const std::string& cmd);
};