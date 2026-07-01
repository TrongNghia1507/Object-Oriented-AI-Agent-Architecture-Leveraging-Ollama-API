#pragma once
#include "../tools/tool.h"
#include <string>

class Environment {
public:
    virtual ~Environment() = default;
    
    // Hàm này nhận vào BẤT KỲ Tool nào và tham số của nó
    virtual std::string runTool(Tool* tool, const std::string& args) = 0;
};

class NativeEnvironment : public Environment {
public:
    std::string runTool(Tool* tool, const std::string& args) override;
};

class SandboxEnvironment : public Environment {
public:
    std::string runTool(Tool* tool, const std::string& args) override;
};