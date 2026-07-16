#include "SystemUtils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include "base64.h"
namespace fs = std::filesystem;
using namespace std;
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
    string encodeImageToBase64(const string& filePath) {
        ifstream file(filePath, ios::binary);
        if (!file.is_open()) {
            cerr << "Error: Cannot open image file at " << filePath << endl;
            return "";
        }
        ostringstream ostrm;
        ostrm << file.rdbuf();
        string binaryData = ostrm.str();
        return base64_encode(reinterpret_cast<const unsigned char*>(binaryData.data()), binaryData.size());
        }
}