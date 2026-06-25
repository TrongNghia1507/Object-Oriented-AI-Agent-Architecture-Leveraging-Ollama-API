#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory> // Thêm thư viện này để dùng std::shared_ptr và std::make_shared
#include "LLMClient/json.hpp" 
#include "utils/SystemUtils.h"
#include "tools/memory_tool.h" // Include file bộ nhớ của bạn
#include "LLMClient/OllamaClient.h" 

using json = nlohmann::json;
using namespace std;

string readFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) return "";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Hàm phụ trợ: Lọc JSON siêu an toàn (Tránh AI ngứa miệng)
string extractCleanJson(const string& rawResponse) {
    size_t firstBrace = rawResponse.find('{');
    size_t lastBrace = rawResponse.rfind('}');
    
    if (firstBrace != string::npos && lastBrace != string::npos && lastBrace > firstBrace) {
        return rawResponse.substr(firstBrace, lastBrace - firstBrace + 1);
    }
    throw runtime_error("Khong tim thay JSON hop le trong cau tra loi!");
}

int main() {
    cout << "=== KHOI DONG HE THONG TEST AGENT - MEMORY TOOLS ===\n\n";

    // BƯỚC 1: NẠP BỐI CẢNH VÀ ĐỌC LUẬT
    string systemContext = SystemUtils::getSystemContext();
    string memoryRules = readFile("skills/memory_planner.md"); 
    
    // Khởi tạo Database Manager: Vì các Tool bộ nhớ cần dùng chung DB kết nối
    auto dbManager = make_shared<DatabaseManager>();

    // BƯỚC 2: RÁP PROMPT
    // KỊCH BẢN 1: Test lưu trữ
    string userQuestion = "Mật khẩu wifi phòng lab là gì?";
    
    // KỊCH BẢN 2: Test tìm kiếm (Bạn có thể comment Kịch bản 1 và mở comment dòng dưới để test Search)
    // string userQuestion = "Bạn có nhớ mật khẩu wifi phòng lab là gì không?";
    
    // Ráp Prompt và thêm một câu ép buộc xuất JSON để code C++ parse không bị lỗi
    string finalPrompt = 
    "Bạn là AI Agent.\n" + 
    systemContext + 
    memoryRules + 
    "\n\nYêu cầu của người dùng: " + userQuestion + 
    "\n\nLƯU Ý QUAN TRỌNG VỀ ĐỊNH DẠNG JSON:\n" +
    "1. BẮT BUỘC trả lời định dạng JSON hợp lệ.\n" +
    "2. Trường \"action\" là tên công cụ.\n" +
    "3. Trường \"arguments\" BẮT BUỘC phải là một CHUỖI (string) thuần túy, KHÔNG ĐƯỢC là một object (không có ngoặc nhọn bao quanh nội dung).\n" +
    "Ví dụ đúng: {\"action\": \"memory_save\", \"arguments\": \"[personal_info] | Mật khẩu wifi là 123\"}\n" +
    "Ví dụ SAI: {\"action\": \"memory_save\", \"arguments\": {\"[personal_info] | ...\"}}";

    cout << "--- PROMPT SE GUI CHO AI ---\n" << finalPrompt << "\n----------------------------\n\n";

    // BƯỚC 3: KHỞI TẠO CLIENT VÀ GỌI MODEL THẬT
    cout << "[3] Đang gửi API tới Ollama...\n";
    
    // Nhớ cấu hình Model và Token chính xác
    OllamaClient client = OllamaClient::withGistDiscovery(
        "qwen2.5vl:3b", 
        "b54e95a50b8b9f0e7743a22593bc0503", 
        "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"
    );
    
    auto apiResult = client.sendPrompt(finalPrompt);

    if (!apiResult.has_value()) {
        cerr << "[LỖI MẠNG/API]: Không thể kết nối hoặc Model trả về lỗi!\n";
        return 1; 
    }

    string rawLlmResponse = apiResult.value();
    cout << "\n[MODEL RAW RESPONSE]:\n" << rawLlmResponse << "\n\n";

    // BƯỚC 4: BÓC TÁCH JSON VÀ GỌI TOOL
    try {
        cout << "[4] Đang làm sạch câu trả lời và Parse JSON...\n";
        string cleanJson = extractCleanJson(rawLlmResponse);
        json responseObj = json::parse(cleanJson);
        
        string actionName = responseObj["action"].get<string>();
        
        // Điều hướng Action: Gọi đúng Tool
        if (actionName == "memory_save") {
            cout << "\n>>> LLM yeu cau chay MemorySaveTool! Khoi tao Tool...\n";
            MemorySaveTool memorySaveTool(dbManager);
            
            // Gọi hàm execute truyền json payload
            string toolResult = memorySaveTool.execute(responseObj);
            cout << "\n[KET QUA TRA VE TU DATABASE]:\n" << toolResult << "\n";
            
        } else if (actionName == "memory_search") {
            cout << "\n>>> LLM yeu cau chay MemorySearchTool! Khoi tao Tool...\n";
            MemorySearchTool memorySearchTool(dbManager);
            
            string toolResult = memorySearchTool.execute(responseObj);
            cout << "\n[KET QUA TRA VE TU DATABASE]:\n" << toolResult << "\n";
            
        } else {
            cout << "LLM goi Tool khac: " << actionName << "\n";
        }

    } catch (const exception& e) {
        cerr << "\n[LỖI NGHIÊM TRỌNG]: Lỗi Parse JSON! Có thể Model trả về sai định dạng.\nChi tiết lỗi: " << e.what() << "\n";
    }

    return 0;
}