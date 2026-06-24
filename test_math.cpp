#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include "LLMClient/json.hpp" 
#include "utils/SystemUtils.h"
#include "tools/calculator_tool.h"
#include "tools/advanced_math_tool.h"
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
    cout << "=== KHOI DONG HE THONG TEST AGENT - MATH TOOLS ===\n\n";

    // BƯỚC 1: NẠP BỐI CẢNH VÀ ĐỌC LUẬT
    string systemContext = SystemUtils::getSystemContext();
    string mathRules = readFile("skills/math_planner.md"); 
    
    if (mathRules.empty()) {
        cerr << "[CANH BAO]: File skills/math_planner.md trong hoặc khong tim thay!\n";
    }

    // BƯỚC 2: RÁP PROMPT & LỰA CHỌN KỊCH BẢN TEST
    
    // KỊCH BẢN 1: Toán cơ bản -> AI phải tự định tuyến sang công cụ "calculator"
    string userQuestion = "Tính giúp tôi giá trị của biểu thức này: (123 + 45) * 12";
    
    // KỊCH BẢN 2: Toán cao cấp -> AI phải tự định tuyến sang công cụ "advanced_math"
    // string userQuestion = "Tính căn bậc hai của số 144 rồi cộng thêm với 2 lũy thừa 3.";
    
    // Ráp Prompt với cơ chế ép khuôn JSON nghiêm ngặt để chống AI nhả Object lỗi
    string finalPrompt = 
        "Bạn là AI Agent.\n" + 
        systemContext + 
        mathRules + 
        "\n\nYêu cầu của người dùng: " + userQuestion + 
        "\n\nLƯU Ý QUAN TRỌNG VỀ ĐỊNH DẠNG JSON:\n" +
        "1. BẮT BUỘC chỉ trả lời dưới dạng định dạng JSON hợp lệ chứa 2 trường \"action\" và \"arguments\".\n" +
        "2. Trường \"arguments\" BẮT BUỘC phải là một CHUỖI VĂN BẢN (String) chứa biểu thức toán học sạch, KHÔNG ĐƯỢC phép bọc nó trong một object (Không được chứa dấu ngoặc nhọn nội bộ).\n" +
        "Ví dụ ĐÚNG: {\"action\": \"calculator\", \"arguments\": \"(15+5)*17\"}\n" +
        "Ví dụ SAI: {\"action\": \"calculator\", \"arguments\": {\"(15+5)*17\"}}\n" +
        "Hãy suy nghĩ thật kỹ để không vi phạm luật định dạng JSON.";

    cout << "--- PROMPT SE GUI CHO AI ---\n" << finalPrompt << "\n----------------------------\n\n";

    // BƯỚC 3: KHỞI TẠO CLIENT VÀ GỌI MODEL THẬT (GIST DISCOVERY)
    cout << "[3] Đang gửi API tới Ollama...\n";
    
    // Sử dụng lại Gist Discovery với PAT Token classic của bạn
    OllamaClient client = OllamaClient::withGistDiscovery(
        "qwen2.5vl:3b", 
        "b54e95a50b8b9f0e7743a22593bc0503", 
        "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" // Hãy thay bằng token ghp_... (classic) đang hoạt động của bạn
    );
    
    auto apiResult = client.sendPrompt(finalPrompt);

    if (!apiResult.has_value()) {
        cerr << "[LỖI MẠNG/API]: Không thể kết nối hoặc Model trả về lỗi!\n";
        return 1; 
    }

    string rawLlmResponse = apiResult.value();
    cout << "\n[MODEL RAW RESPONSE]:\n" << rawLlmResponse << "\n\n";

    // BƯỚC 4: BÓC TÁCH JSON VÀ GỌI TOOL (ÁP DỤNG CƠ CHẾ PARSING PHÒNG THỦ PHÁT HIỆN LỖI)
    try {
        cout << "[4] Đang làm sạch câu trả lời và Parse JSON...\n";
        string cleanJson = extractCleanJson(rawLlmResponse);
        json responseObj = json::parse(cleanJson);
        
        string actionName = responseObj["action"].get<string>();
        
        // --- Xử lý Arguments phòng thủ chống AI nhả Object rác ---
        string args;
        if (responseObj["arguments"].is_object()) {
            cout << "[CANH BAO]: AI dung sai dinh dang Object! Tien hanh tu dong sua sai...\n";
            args = responseObj["arguments"].begin().key(); // Lấy key thô từ object rác
        } else {
            args = responseObj["arguments"].get<string>(); // Lấy string chuẩn
        }
        
        // Tạo payload chuẩn hóa để đưa vào Tool
        json toolPayload = { {"action", actionName}, {"arguments", args} };
        cout << "-> Tên công cụ: " << actionName << "\n";
        cout << "-> Tham số biểu thức: " << args << "\n";

        // Điều hướng gọi Tool toán học chính xác
        if (actionName == "calculator") {
            cout << "\n>>> Kích hoạt CalculatorTool...\n";
            CalculatorTool calcTool;
            string toolResult = calcTool.execute(toolPayload);
            cout << "\n[KET QUA TRA VE TU CALCULATOR]:\n" << toolResult << "\n";
            
        } else if (actionName == "advanced_math") {
            cout << "\n>>> Kích hoạt AdvancedMathTool...\n";
            AdvancedMathTool advMathTool;
            string toolResult = advMathTool.execute(toolPayload);
            cout << "\n[KET QUA TRA VE TU ADVANCED_MATH]:\n" << toolResult << "\n";
            
        } else {
            cout << "[CANH BAO]: LLM goi cong cu la hoac khong thuoc pham vi toan hoc: " << actionName << "\n";
        }

    } catch (const exception& e) {
        cerr << "\n[LỖI NGHIÊM TRỌNG]: Quá trình Parse JSON hoặc gọi Tool thất bại!\n";
        cerr << "Chi tiết lỗi: " << e.what() << "\n";
    }

    return 0;
}
