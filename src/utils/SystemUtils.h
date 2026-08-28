#include <string>

namespace SystemUtils {
    // Hàm lấy thông tin hệ điều hành và đường dẫn hiện tại
    std::string getSystemContext();
    std::string selectImageFromFolder(const std::string& folderPath);
    std::string encodeImageToBase64(const std::string& filePath);
    std::string unescapeContent(const std::string& input);
}