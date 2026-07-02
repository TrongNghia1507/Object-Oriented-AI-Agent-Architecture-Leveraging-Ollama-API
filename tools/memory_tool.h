#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "tool.h"
#include <sqlite3.h>
#include <string>
#include <memory>

// Lớp quản lý Database
class DatabaseManager {
private:
    sqlite3* db;
public:
    // SỬA LỖI HARDCODE: Constructor nhận vào một đường dẫn động để tạo file DB cô lập theo từng Task
    explicit DatabaseManager(const std::string& dbPath = "memory.db");
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
        return "Lưu trí nhớ. Format bắt buộc: [TAG] | NỘI DUNG. Ví dụ: [math] | 15*3=45"; 
    }
    
    std::string execute(const std::string& arguments) override;
};

// Tool: Tìm kiếm trí nhớ
class MemorySearchTool : public Tool {
private:
    std::shared_ptr<DatabaseManager> dbManager;
public:
    MemorySearchTool(std::shared_ptr<DatabaseManager> dbManager);
    std::string getName() const override { return "memory_search"; }
    
    std::string getDescription() const override { 
        return "Tìm kiếm trí nhớ bằng từ khóa."; 
    }
    
    std::string execute(const std::string& arguments) override;
};

#endif