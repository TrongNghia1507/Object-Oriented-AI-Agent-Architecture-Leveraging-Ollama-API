#pragma once
#include <string>

class Environment {
public:
    virtual ~Environment() = default;
    // Hàm này trả về câu lệnh "cd ..." nếu là Sandbox, hoặc rỗng nếu là Native
    virtual std::string getDirectoryPrefix() = 0; 
};

class NativeEnvironment : public Environment {
public:
    std::string getDirectoryPrefix() override {
        return "";//native thì ko cần truy cập folder sandbox
    }
};

class SandboxEnvironment : public Environment {
private:
    std::string sandboxDir;
public:
    SandboxEnvironment(const std::string& dir = "./sandbox_root") : sandboxDir(dir) {}
    
    std::string getDirectoryPrefix() override {
        #ifdef _WIN32
            return "cd /d " + sandboxDir + " && ";
        #else
            return "cd " + sandboxDir + " && ";
        #endif
    }
};