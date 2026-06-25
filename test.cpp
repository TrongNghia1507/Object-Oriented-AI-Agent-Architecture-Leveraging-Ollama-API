#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "LLMClient/json.hpp" 
#include "utils/SystemUtils.h"
#include "tools/exec_tool.h"

// BỔ SUNG INCLUDE FILE CLIENT CỦA BRO VÀO ĐÂY
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

// 2. Hàm phụ trợ: Lọc JSON siêu an toàn (Tránh AI ngứa miệng)
string extractCleanJson(const string& rawResponse) {
    size_t firstBrace = rawResponse.find('{');
    size_t lastBrace = rawResponse.rfind('}');
    
    if (firstBrace != string::npos && lastBrace != string::npos && lastBrace > firstBrace) {
        return rawResponse.substr(firstBrace, lastBrace - firstBrace + 1);
    }
    throw runtime_error("Khong tim thay JSON hop le trong cau tra loi!");
}

int main() {
    cout << "=== KHOI DONG HE THONG TEST AGENT (REAL MODEL) ===\n\n";

    // BƯỚC 1: NẠP BỐI CẢNH VÀ ĐỌC LUẬT
    string systemContext = SystemUtils::getSystemContext();
    string execRules = readFile("skills/exec_planner.md"); 
    
    // BƯỚC 2: RÁP PROMPT
    string userQuestion = "xoá file name.txt trong thư mục results";
    
    string finalPrompt = 
        "Bạn là AI Agent.\n" + 
        systemContext + 
        execRules + 
        "\n\nYêu cầu của người dùng: " + userQuestion;

    cout << "--- PROMPT SE GUI CHO AI ---\n" << finalPrompt << "\n----------------------------\n\n";

    // BƯỚC 3: KHỞI TẠO CLIENT VÀ GỌI MODEL THẬT
    cout << "[3] Đang gửi API tới Ollama...\n";
    
    // Khởi tạo OllamaClient (Bro nhớ đổi tên model cho đúng với máy bro đang chạy nhé, vd: qwen2, llama3...)
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
    
    // Gọi hàm sendPrompt. Vì bro dùng std::expected nên phải check lỗi đàng hoàng
    auto apiResult = client.sendPrompt(finalPrompt);

    if (!apiResult.has_value()) {
        cerr << "[LỖI MẠNG/API]: Không thể kết nối hoặc Model trả về lỗi!\n";
        return 1; // Ngắt chương trình nếu API toang
    }

    // Lấy chuỗi raw text thật từ Model
    string rawLlmResponse = apiResult.value();
    cout << "\n[MODEL RAW RESPONSE]:\n" << rawLlmResponse << "\n\n";

    // BƯỚC 4: BÓC TÁCH JSON VÀ GỌI TOOL
    try {
        cout << "[4] Đang làm sạch câu trả lời và Parse JSON...\n";
        string cleanJson = extractCleanJson(rawLlmResponse);
        json responseObj = json::parse(cleanJson);
        
        string actionName = responseObj["action"].get<string>();
        
        if (actionName == "exec") {
            cout << "\n>>> LLM yeu cau chay ExecTool! Khoi tao Tool...\n";
            ExecTool execTool;
            
            string toolResult = execTool.execute(responseObj);
            
            cout << "\n[KET QUA TRA VE TU HỆ ĐIỀU HÀNH]:\n" << toolResult << "\n";
        } else {
            cout << "LLM goi Tool khac: " << actionName << "\n";
        }

    } catch (const exception& e) {
        cerr << "\n[LỖI NGHIÊM TRỌNG]: Lỗi Parse JSON! Có thể Model trả về sai định dạng.\nChi tiết lỗi: " << e.what() << "\n";
    }

    return 0;
}