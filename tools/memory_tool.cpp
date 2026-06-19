#include "memory_tool.h"
#include <iostream>

// =====================================================================
// 1. LỚP QUẢN LÝ DATABASE (Áp dụng nguyên tắc RAII chống rò rỉ bộ nhớ)
// =====================================================================

DatabaseManager::DatabaseManager() : db(nullptr) {
    // Bước 1: Agent nhấc máy gọi chú thủ thư (Mở file db)
    if (sqlite3_open("memory.db", &db) != SQLITE_OK) {
        std::cerr << "Lỗi mở DB: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Khởi tạo bảng ngay khi mở kết nối (Câu thần chú Kẻ Bảng)
    const char* create_table_sql = 
        "CREATE TABLE IF NOT EXISTS agent_memory ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "content TEXT NOT NULL, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
    
    char* errMsg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Lỗi tạo bảng: " << errMsg << std::endl;
        sqlite3_free(errMsg); // Giải phóng bộ nhớ chuỗi lỗi của SQLite
    }
}

DatabaseManager::~DatabaseManager() {
    // Tự động cúp điện thoại khi Agent bị hủy, không bao giờ lo Memory Leak
    if (db) {
        sqlite3_close(db);
    }
}

// =====================================================================
// 2. TOOL MEMORY SAVE (Hành động cất ký ức vào sổ)
// =====================================================================

MemorySaveTool::MemorySaveTool(std::shared_ptr<DatabaseManager> dbMgr) 
    : dbManager(dbMgr) {}

// SỬA Ở ĐÂY: Bỏ std::optional, chỉ giữ std::string
std::string MemorySaveTool::execute(const std::string& args) {
    if (args.empty()) return "Lỗi: Dữ liệu trống, không có gì để lưu."; // Trả về chuỗi thông báo lỗi thay vì nullopt

    sqlite3* db = dbManager->getDB();
    sqlite3_stmt* stmt;
    
    // Câu thần chú Lưu Trữ với dấu ? để chống SQL Injection
    const char* sql = "INSERT INTO agent_memory (content) VALUES (?);";

    // Bước 2: Biên dịch lệnh
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "Lỗi chuẩn bị SQL: " + std::string(sqlite3_errmsg(db));
    }

    // Nhét dữ liệu của Agent vào vị trí dấu ? (Tham số 1)
    sqlite3_bind_text(stmt, 1, args.c_str(), -1, SQLITE_STATIC);

    // Bước 3: Thực thi hành động lưu
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return "Lỗi lưu dữ liệu: " + std::string(sqlite3_errmsg(db));
    }

    // Bước 4: Dọn dẹp câu lệnh
    sqlite3_finalize(stmt);
    return "Đã lưu vào bộ nhớ thành công."; // Báo cáo lại cho LLM biết đã xong
}

// =====================================================================
// 3. TOOL MEMORY SEARCH (Hành động tìm ký ức)
// =====================================================================

MemorySearchTool::MemorySearchTool(std::shared_ptr<DatabaseManager> dbMgr) 
    : dbManager(dbMgr) {}

// SỬA Ở ĐÂY: Bỏ std::optional, chỉ giữ std::string
std::string MemorySearchTool::execute(const std::string& args) {
    if (args.empty()) return ""; // Trả về chuỗi rỗng thay vì nullopt

    sqlite3* db = dbManager->getDB();
    sqlite3_stmt* stmt;
    
    // Câu thần chú Tìm Kiếm. LIMIT 1 để chỉ lấy ký ức mới nhất nếu có nhiều dòng trùng
    const char* sql = "SELECT content FROM agent_memory WHERE content LIKE ? ORDER BY created_at DESC LIMIT 1;";

    // Bước 2: Biên dịch lệnh
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Lỗi chuẩn bị tìm kiếm: " << sqlite3_errmsg(db) << std::endl;
        return ""; // Trả về chuỗi rỗng thay vì nullopt
    }

    // Sức mạnh của toán tử LIKE: Kẹp từ khóa vào giữa 2 dấu %
    std::string keyword = "%" + args + "%";
    
    // SQLITE_TRANSIENT báo cho SQLite biết phải tự copy chuỗi này vì std::string có thể bị hủy
    sqlite3_bind_text(stmt, 1, keyword.c_str(), -1, SQLITE_TRANSIENT);

    std::string result = "";
    
    // Bước 3: Đọc từng dòng kết quả (Ở đây giới hạn 1 dòng nên chỉ cần if)
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // Rút nội dung chữ ở Cột 0 (cột content)
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            result = reinterpret_cast<const char*>(text);
        }
    }

    // Bước 4: Dọn dẹp
    sqlite3_finalize(stmt);

    // Kịch bản Caching: Nếu DB rỗng (Cache Miss) -> trả về "" (chuỗi rỗng) để Agent tự đi tính toán
    if (result.empty()) {
        return ""; 
    }
    
    // Cache Hit -> Trả về kết quả cho LLM
    return result; 
}