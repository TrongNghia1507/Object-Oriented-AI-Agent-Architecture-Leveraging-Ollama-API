#include "SystemUtils.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace SystemUtils {

    std::string getSystemContext() {
        std::string currentPath = fs::current_path().string();
        
        std::string osName;
        #ifdef _WIN32
            osName = "Windows";
        #else
            osName = "Linux/Ubuntu";
        #endif

        return "--- TRẠNG THÁI HỆ THỐNG ---\n"
               "- Hệ điều hành: " + osName + "\n"
               "- Thư mục làm việc hiện tại (CWD): " + currentPath + "\n"
               "---------------------------\n\n";
    }

}