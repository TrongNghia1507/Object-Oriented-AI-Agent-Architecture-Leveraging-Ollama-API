#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "tool.h" 
#include <memory>
#include <string>
#include <sqlite3.h>

// Lớp quản lý Database
class DatabaseManager {
private:
    sqlite3* db;

public:
    // Đặt giá trị mặc định là "memory.db"
    explicit DatabaseManager(const std::string& dbPath = "memory.db");
    ~DatabaseManager();

    sqlite3* getDB() const {
        return db;
    }
};

// Tool: Lưu trữ trí nhớ
class MemorySaveTool : public Tool { // Kế thừa từ base class Tool
private:
    std::shared_ptr<DatabaseManager> dbManager;

public:
    explicit MemorySaveTool(std::shared_ptr<DatabaseManager> dbMgr);

    // Override các hàm của interface Tool
    std::string getName() const override { 
        return "memory_save"; 
    }
    
    std::string getDescription() const override {
        return "Lưu trí nhớ. Format bắt buộc: [TAG] | NỘI DUNG. Ví dụ: [math] | 15*3=45";
    }

    std::string execute(const std::string& arguments) override;
};

// Tool: Tìm kiếm trí nhớ
class MemorySearchTool : public Tool { // Kế thừa từ base class Tool
private:
    std::shared_ptr<DatabaseManager> dbManager;

public:
    explicit MemorySearchTool(std::shared_ptr<DatabaseManager> dbMgr);

    // Override các hàm của interface Tool
    std::string getName() const override { 
        return "memory_search"; 
    }
    
    std::string getDescription() const override {
        return "Tìm kiếm trí nhớ bằng từ khóa. Format: [TAG] | TỪ KHÓA (nhập general nếu không có tag cụ thể).";
    }

    std::string execute(const std::string& arguments) override;
};

#endif