#include "SystemUtils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
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

// Hàm tạo menu chọn ảnh
    std::string selectImageFromFolder(const std::string& folderPath) {
        std::vector<fs::path> imagePaths;
        
        // 1. Quét thư mục và lọc các file ảnh
        if (fs::exists(folderPath) && fs::is_directory(folderPath)) {
            for (const auto& entry : fs::directory_iterator(folderPath)) {
                auto ext = entry.path().extension().string();
                // Lọc định dạng ảnh cơ bản (có thể thêm .jpeg, .webp...)
                if (ext == ".png" || ext == ".jpg") {
                    imagePaths.push_back(entry.path());
                }
            }
        } else {
            std::cerr << "[Lỗi] Thư mục " << folderPath << " không tồn tại!\n";
            return "";
        }

        if (imagePaths.empty()) {
            std::cerr << "[Info] Không tìm thấy ảnh nào trong folder.\n";
            return "";
        }

        // 2. In Menu cho người dùng
        std::cout << "\n=== DANH SÁCH ẢNH TEST ===\n";
        cout << "0. Không cần chọn ảnh\n";
        for (size_t i = 0; i < imagePaths.size(); ++i) {
            std::cout << i + 1 << ". " << imagePaths[i].filename().string() << "\n";
        }

        // 3. Xử lý Input
        int choice = -1;
        std::cout << "Chọn ảnh (Nhập số): ";
        std::cin >> choice;

        if (choice > 0 && choice <= imagePaths.size()) {
            // Lấy đường dẫn tuyệt đối (Absolute Path) để đảm bảo không bị lỗi path tương đối
            return fs::absolute(imagePaths[choice - 1]).string(); 
        } else {
            return "";
        }
    }
}