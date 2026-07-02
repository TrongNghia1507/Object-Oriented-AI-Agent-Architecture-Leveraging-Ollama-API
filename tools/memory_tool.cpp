#include "memory_tool.h"
#include <iostream>
#include <chrono>   
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string_view> // Thêm thư viện Modern C++17 để tối ưu hóa xử lý chuỗi

// Hàm hỗ trợ lấy thời gian thực của máy tính (Local Time)
std::string getCurrentLocalTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
    // Sử dụng hàm an toàn hơn trên Windows/Linux hoặc bọc luồng an toàn
#ifdef _WIN32
    std::tm local_tm;
    localtime_s(&local_tm, &now_time);
    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
#else
    std::tm* local_tm = std::localtime(&now_time);
    std::stringstream ss;
    ss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
#endif
    return ss.str();
}

// ============================================================================
// 1. LỚP QUẢN LÝ DATABASE (ĐÃ ĐƯỢC CÔ LẬP ĐƯỜNG DẪN)
// ============================================================================

// SỬA LỖI: Mở cơ sở dữ liệu dựa trên đường dẫn dbPath được tiêm từ ngoài vào
DatabaseManager::DatabaseManager(const std::string& dbPath) : db(nullptr) {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Lỗi mở DB tại vị trí [" << dbPath << "]: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    const char* create_table_sql = 
        "CREATE TABLE IF NOT EXISTS agent_memory ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "context_tag TEXT DEFAULT 'general', "
        "content TEXT NOT NULL, "
        "created_at DATETIME);";
    
    sqlite3_exec(db, create_table_sql, nullptr, nullptr, nullptr);
}

DatabaseManager::~DatabaseManager() {
    if (db) sqlite3_close(db);
}

// ============================================================================
// 2. TOOL MEMORY SAVE (Xử lý bóc tách Tag & Ghi dữ liệu)
// ============================================================================

MemorySaveTool::MemorySaveTool(std::shared_ptr<DatabaseManager> dbMgr) : dbManager(dbMgr) {}

std::string MemorySaveTool::execute(const std::string& arguments) {
    if (arguments.empty()) return "Lỗi: Tham số trống.";

    std::string tag = "general"; 
    std::string content = arguments;
    
    // TỐI ƯU HÓA C++17: Sử dụng std::string_view để duyệt tìm ký tự cắt chuỗi mà không tạo bản sao rác trên RAM
    std::string_view args_view(arguments);
    size_t pos = args_view.find("] | ");
    
    if (!args_view.empty() && args_view.front() == '[' && pos != std::string_view::npos) {
        tag = std::string(args_view.substr(1, pos - 1));       
        content = std::string(args_view.substr(pos + 4));      
    }

    std::string currentTime = getCurrentLocalTime();
    sqlite3* db = dbManager->getDB();
    if (!db) return "Lỗi: Cơ sở dữ liệu chưa được khởi tạo thành công!";

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO agent_memory (context_tag, content, created_at) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, currentTime.c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        return "Đã lưu thành công vào cơ sở dữ liệu tách biệt. Nhãn: [" + tag + "] vào lúc " + currentTime;
    }
    return "Lỗi thực thi truy vấn cơ sở dữ liệu!";
}

// ============================================================================
// 3. TOOL MEMORY SEARCH (Truy xuất kèm Tag và Thời gian)
// ============================================================================

MemorySearchTool::MemorySearchTool(std::shared_ptr<DatabaseManager> dbMgr) : dbManager(dbMgr) {}

std::string MemorySearchTool::execute(const std::string& arguments) {
    if (arguments.empty()) return "Lỗi: Từ khóa tìm kiếm trống.";

    sqlite3* db = dbManager->getDB();
    if (!db) return "Lỗi: Cơ sở dữ liệu chưa được kết nối!";

    sqlite3_stmt* stmt;
    const char* sql = "SELECT context_tag, content, created_at FROM agent_memory WHERE content LIKE ? ORDER BY created_at DESC LIMIT 1;";
    
    std::string result = "";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string keyword = "%" + arguments + "%";
        sqlite3_bind_text(stmt, 1, keyword.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* tag_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            const char* content_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* time_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            
            result = "[BỐI CẢNH TRÍ NHỚ ĐÃ TRÍCH XUẤT]\n";
            result += "- Thời gian lưu: " + std::string(time_text ? time_text : "Không rõ") + "\n";
            result += "- Nhãn phân loại: [" + std::string(tag_text ? tag_text : "general") + "]\n";
            result += "- Nội dung trí nhớ: " + std::string(content_text ? content_text : "");
        }
        sqlite3_finalize(stmt);
    }
    
    return result.empty() ? "Không tìm thấy ký ức nào trùng khớp với từ khóa." : result; 
}