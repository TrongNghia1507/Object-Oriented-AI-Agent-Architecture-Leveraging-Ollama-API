#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "LLMClient/json.hpp"
#include "utils/SystemUtils.h"
#include "tools/translate_tool.h" // Include file translate tool
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
    cout << "=== KHOI DONG HE THONG TEST AGENT - TRANSLATE TOOL ===\n\n";

    // BƯỚC 1: NẠP BỐI CẢNH VÀ ĐỌC LUẬT
    string systemContext  = SystemUtils::getSystemContext();
    string translateRules = readFile("skills/translate_planner.md");

    if (translateRules.empty()) {
        cerr << "[CANH BAO]: File skills/translate_planner.md trống hoặc không tìm thấy!\n";
    }

    // BƯỚC 2: RÁP PROMPT & LỰA CHỌN KỊCH BẢN TEST
    // KỊCH BẢN TEST: Người dùng yêu cầu dịch trực tiếp một câu Tiếng Việt sang Tiếng Anh
    // (mô phỏng đúng bước 1 của quy trình "translate -> web_search" ở translate_planner.md)
    string userQuestion = "Thủ đô của nước Pháp là gì?";

    // Ráp Prompt với cơ chế ép khuôn JSON nghiêm ngặt để chống AI nhả Object lỗi
    string finalPrompt =
    "Bạn là AI Agent.\n" +
    systemContext +
    translateRules +
    "\n\nYêu cầu của người dùng: " + userQuestion +
    "\n\nLƯU Ý QUAN TRỌNG VỀ ĐỊNH DẠNG JSON:\n" +
    "1. BẮT BUỘC trả lời định dạng JSON hợp lệ chứa 2 trường \"action\" và \"arguments\".\n" +
    "2. Trường \"action\" phải là \"translate\" (vì đây là bước dịch từ khóa sang Tiếng Anh).\n" +
    "3. Trường \"arguments\" BẮT BUỘC phải là một CHUỖI (string) thuần túy chứa câu/từ khóa cần dịch, KHÔNG ĐƯỢC là một object.\n" +
    "Ví dụ đúng: {\"action\": \"translate\", \"arguments\": \"Thủ đô của nước Pháp\"}\n" +
    "Ví dụ SAI: {\"action\": \"translate\", \"arguments\": {\"text\": \"Thủ đô của nước Pháp\"}}";

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

        cout << "-> Tên công cụ: " << actionName << "\n";
        cout << "-> Văn bản cần dịch: " << args << "\n";

        // Điều hướng Action: Gọi đúng Tool
        // LƯU Ý: TranslateTool::execute() nhận CHUỖI THUẦN (đúng chữ ký Tool::execute),
        // nên ta truyền thẳng `args` (không bọc lại thành json) để tool chạy đúng thật sự.
        if (actionName == "translate") {
            cout << "\n>>> Kích hoạt TranslateTool (MyMemory API)...\n";
            TranslateTool translateTool; // Khởi tạo Tool không cần DB

            string toolResult = translateTool.execute(args);
            cout << "\n[KET QUA DICH THUAT]:\n" << toolResult << "\n";

            cout << "\n>>> Goi y: dung chinh phan ban dich Tieng Anh o tren lam Arguments \n"
                 << "    cho buoc tiep theo (Action: web_search) neu muon tra cuu thong tin.\n";

        } else {
            cout << "LLM goi Tool khac: " << actionName << "\n";
        }

    } catch (const exception& e) {
        cerr << "\n[LỖI NGHIÊM TRỌNG]: Lỗi Parse JSON hoặc Gọi Tool!\nChi tiết lỗi: " << e.what() << "\n";
    }

    return 0;
}