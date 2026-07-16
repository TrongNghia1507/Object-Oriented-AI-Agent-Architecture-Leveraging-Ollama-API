#include <string>

// Dùng namespace (tùy chọn) để gom nhóm các hàm tiện ích lại cho chuyên nghiệp
namespace SystemUtils {
    // Hàm lấy thông tin hệ điều hành và đường dẫn hiện tại
    std::string getSystemContext();
    
    std::string encodeImageToBase64(const std::string& filePath);
}