#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "LLMClient/json.hpp" 
#include "utils/SystemUtils.h"
#include "tools/web_search_tool.h" // Include file web search tool
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
    cout << "=== KHOI DONG HE THONG TEST AGENT - WEB SEARCH TOOL ===\n\n";

    // BƯỚC 1: NẠP BỐI CẢNH VÀ ĐỌC LUẬT
    string systemContext = SystemUtils::getSystemContext();
    string searchRules = readFile("skills/web_search_planner.md"); 
    
    if (searchRules.empty()) {
        cerr << "[CANH BAO]: File skills/web_search_planner.md trống hoặc không tìm thấy!\n";
    }

    // BƯỚC 2: RÁP PROMPT & LỰA CHỌN KỊCH BẢN TEST
    // KỊCH BẢN TEST: Hỏi một sự kiện/thông tin thực tế để AI gọi DuckDuckGo
    string userQuestion = "RONALDO";
    
    // Ráp Prompt với cơ chế ép khuôn JSON nghiêm ngặt để chống AI nhả Object lỗi
    string finalPrompt = 
    "Bạn là AI Agent.\n" + 
    systemContext + 
    searchRules + 
    "\n\nYêu cầu của người dùng: " + userQuestion + 
    "\n\nLƯU Ý QUAN TRỌNG VỀ ĐỊNH DẠNG JSON:\n" +
    "1. BẮT BUỘC trả lời định dạng JSON hợp lệ.\n" +
    "2. Trường \"action\" là tên công cụ.\n" +
    "3. Trường \"arguments\" BẮT BUỘC phải là một CHUỖI (string) thuần túy chứa TỪ KHÓA TÌM KIẾM, KHÔNG ĐƯỢC là một object.\n" +
    "Ví dụ đúng: {\"action\": \"web_search\", \"arguments\": \"Thủ đô nước Pháp\"}\n" +
    "Ví dụ SAI: {\"action\": \"web_search\", \"arguments\": {\"keyword\": \"Thủ đô nước Pháp\"}}";

    cout << "--- PROMPT SE GUI CHO AI ---\n" << finalPrompt << "\n----------------------------\n\n";

    // BƯỚC 3: KHỞI TẠO CLIENT VÀ GỌI MODEL THẬT
    cout << "[3] Đang gửi API tới Ollama...\n";
    
    // Cau hinh qua bien moi truong (KHONG hardcode secret):
    //   export OLLAMA_MODEL="qwen2.5vl:3b"
    //   export OLLAMA_BASE_URL="http://localhost:11434"     (local)
    //   hoac OLLAMA_GIST_ID + OLLAMA_GIST_PAT              (tunnel)
    const char* modelEnv  = std::getenv("OLLAMA_MODEL");
    const char* baseUrl   = std::getenv("OLLAMA_BASE_URL");
    const char* gistId    = std::getenv("OLLAMA_GIST_ID");
    const char* gistPat   = std::getenv("OLLAMA_GIST_PAT");
    if (!modelEnv) { std::cerr << "Can OLLAMA_MODEL\n"; return 1; }
    OllamaClient client = baseUrl
        ? OllamaClient(modelEnv, baseUrl)
        : OllamaClient::withGistDiscovery(modelEnv, gistId ? gistId : "", gistPat ? gistPat : "");
    
    auto apiResult = client.sendPrompt(finalPrompt);

    if (!apiResult.has_value()) {
        cerr << "[LỖI MẠNG/API]: Không thể kết nối hoặc Model trả về lỗi!\n";
        return 1; 
    }

    string rawLlmResponse = apiResult.value();
    cout << "\n[MODEL RAW RESPONSE]:\n" << rawLlmResponse << "\n\n";

    // BƯỚC 4: BÓC TÁCH JSON VÀ GỌI TOOL PHÒNG THỦ
    try {
        cout << "[4] Đang làm sạch câu trả lời và Parse JSON...\n";
        string cleanJson = extractCleanJson(rawLlmResponse);
        json responseObj = json::parse(cleanJson);
        
        string actionName = responseObj["action"].get<string>();
        
        // --- Xử lý Arguments phòng thủ chống AI nhả Object rác ---
        string args;
        if (responseObj["arguments"].is_object()) {
            cout << "[CANH BAO]: AI dung sai dinh dang Object! Tien hanh tu dong sua sai...\n";
            args = responseObj["arguments"].begin().key(); 
        } else {
            args = responseObj["arguments"].get<string>(); 
        }
        
        // Tạo payload chuẩn hóa để đưa vào Tool
        json toolPayload = { {"action", actionName}, {"arguments", args} };
        cout << "-> Tên công cụ: " << actionName << "\n";
        cout << "-> Từ khóa tìm kiếm: " << args << "\n";
        
        // Điều hướng Action: Gọi đúng Tool
        if (actionName == "web_search") {
            cout << "\n>>> Kích hoạt WebSearchTool (DuckDuckGo)...\n";
            WebSearchTool webSearchTool; // Khởi tạo Tool không cần DB
            
            // Gọi hàm execute truyền json payload
            string toolResult = webSearchTool.execute(toolPayload);
            cout << "\n[KET QUA TRA VE TU INTERNET]:\n" << toolResult << "\n";
            
        } else {
            cout << "LLM goi Tool khac: " << actionName << "\n";
        }

    } catch (const exception& e) {
        cerr << "\n[LỖI NGHIÊM TRỌNG]: Lỗi Parse JSON hoặc Gọi Tool!\nChi tiết lỗi: " << e.what() << "\n";
    }

    return 0;
}
