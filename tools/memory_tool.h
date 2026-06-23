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
    
    // Gợi ý cho AI cách truyền tham số chuẩn
    std::string getDescription() const { 
        return "Lưu trí nhớ. Format bắt buộc: [TAG] | NỘI DUNG. Ví dụ: [math] | 15*3=45"; 
    }
    
    std::string execute(const json& payload) override;
};

// Tool: Tìm kiếm trí nhớ
class MemorySearchTool : public Tool {
private:
    std::shared_ptr<DatabaseManager> dbManager;
public:
    MemorySearchTool(std::shared_ptr<DatabaseManager> dbManager);
    std::string getName() const override { return "memory_search"; }
    
    std::string getDescription() const { 
        return "Tìm kiếm trí nhớ bằng từ khóa."; 
    }
    
    std::string execute(const json& payload) override;
};

#endif 