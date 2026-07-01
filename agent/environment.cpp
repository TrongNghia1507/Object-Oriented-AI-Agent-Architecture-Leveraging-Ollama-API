#include "environment.h"
using namespace std;
std::string NativeEnvironment::runTool(Tool* tool, const std::string& args){
        // Chạy THẬT! Gọi thẳng vào hàm execute() của cái tool đó.
        // Bất kể nó là ExecTool, MathTool hay WebTool.
        return tool->execute(args);
}
std::string SandboxEnvironment::runTool(Tool* tool, const std::string& args) {
        return "[SANDBOX] Đã chạy được tool '" + tool->getName() + 
               "'. Lệnh truyền vào: " + args;
}
