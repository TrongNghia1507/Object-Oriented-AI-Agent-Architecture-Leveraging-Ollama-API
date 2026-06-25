#include "memory_tool.h"
#include <iostream>
#include <chrono>   // Thư viện lấy thời gian
#include <ctime>
#include <iomanip>
#include <sstream>
// Hàm hỗ trợ lấy thời gian thực của máy tính (Local Time)
std::string getCurrentLocalTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_time);
    std::stringstream ss;
    ss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}


// 1. LỚP QUẢN LÝ DATABASE

DatabaseManager::DatabaseManager() : db(nullptr) {
    if (sqlite3_open("memory.db", &db) != SQLITE_OK) {
        std::cerr << "Lỗi mở DB: " << sqlite3_errmsg(db) << std::endl;
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


// 2. TOOL MEMORY SAVE (Xử lý bóc tách Tag & Ghi dữ liệu)

MemorySaveTool::MemorySaveTool(std::shared_ptr<DatabaseManager> dbMgr) : dbManager(dbMgr) {}

std::string MemorySaveTool::execute(const std::string& arguments) {
    std::string args = arguments;
    if (args.empty()) return "Lỗi: Tham số trống.";

    // Logic bóc tách Tag và Content từ args (Ví dụ: "[math] | 15*3=45")
    std::string tag = "general"; // Nhãn mặc định nếu AI quên truyền format
    std::string content = args;
    
    size_t pos = args.find("] | ");
    if (args.front() == '[' && pos != std::string::npos) {
        tag = args.substr(1, pos - 1);       // Lấy phần chữ giữa [ và ]
        content = args.substr(pos + 4);      // Lấy phần nội dung phía sau chuỗi "] | "
    }

    // Lấy thời gian thực từ C++
    std::string currentTime = getCurrentLocalTime();

    sqlite3* db = dbManager->getDB();
    sqlite3_stmt* stmt;
    
    // Câu lệnh chèn cả 3 cột: tag, nội dung, thời gian
    const char* sql = "INSERT INTO agent_memory (context_tag, content, created_at) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, currentTime.c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        // Trả về câu thông báo cho AI biết nó vừa lưu thành công kèm Tag
        return "Đã lưu thành công. Nhãn: [" + tag + "] vào lúc " + currentTime;
    }
    return "Lỗi cơ sở dữ liệu!";
}


// 3. TOOL MEMORY SEARCH (Truy xuất kèm Tag và Thời gian)
MemorySearchTool::MemorySearchTool(std::shared_ptr<DatabaseManager> dbMgr) : dbManager(dbMgr) {}

std::string MemorySearchTool::execute(const std::string& arguments) {
    std::string args = arguments;
    if (args.empty()) return "";

    sqlite3* db = dbManager->getDB();
    sqlite3_stmt* stmt;
    
    // Câu lệnh tìm kiếm lấy ra CẢ 3 thông tin để báo lại cho AI
    const char* sql = "SELECT context_tag, content, created_at FROM agent_memory WHERE content LIKE ? ORDER BY created_at DESC LIMIT 1;";
    
    std::string result = "";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string keyword = "%" + args + "%";
        sqlite3_bind_text(stmt, 1, keyword.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* tag_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            const char* content_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* time_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            
            // Format lại kết quả trả về để AI dễ dàng đọc hiểu bối cảnh
            result = "[BỐI CẢNH DỮ LIỆU]\n";
            result += "- Thời gian: " + std::string(time_text ? time_text : "Unknown") + "\n";
            result += "- Nhãn loại: [" + std::string(tag_text ? tag_text : "general") + "]\n";
            result += "- Nội dung: " + std::string(content_text ? content_text : "");
        }
        sqlite3_finalize(stmt);
    }
    
    if (result.empty()) return ""; 
    return result; 
}