#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "tool.h" // Interface gốc của hệ thống
#include <sqlite3.h>
#include <string>
#include <memory>
// Đã bỏ #include <optional> vì không cần dùng nữa

// Lớp cha quản lý kết nối Database (Áp dụng RAII)
class DatabaseManager {
private:
    sqlite3* db;
public:
    DatabaseManager();
    ~DatabaseManager();
    sqlite3* getDB() const { return db; }
};

// Tool: Lưu trữ trí nhớ
class MemorySaveTool : public Tool {
private:
    std::shared_ptr<DatabaseManager> dbManager;
public:
    MemorySaveTool(std::shared_ptr<DatabaseManager> dbManager);
    std::string getName() const override { return "memory_save"; }
    std::string getDescription() const override { 
        return "Lưu thông tin, sự kiện quan trọng vào bộ nhớ cơ sở dữ liệu."; 
    }
    
    // SỬA Ở ĐÂY: Trả về std::string thay vì std::optional<std::string>
    std::string execute(const std::string& args) override;
};

// Tool: Tìm kiếm trí nhớ
class MemorySearchTool : public Tool {
private:
    std::shared_ptr<DatabaseManager> dbManager;
public:
    MemorySearchTool(std::shared_ptr<DatabaseManager> dbManager);
    std::string getName() const override { return "memory_search"; }
    std::string getDescription() const override { 
        return "Lưu thông tin, sự kiện quan trọng vào bộ nhớ cơ sở dữ liệu."; 
    }
    
    // SỬA Ở ĐÂY: Trả về std::string thay vì std::optional<std::string>
    std::string execute(const std::string& args) override;
};

#endif // MEMORY_TOOL_H