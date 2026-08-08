#pragma once
#include "tool.h"
#include <string>
#include <vector>
#include <utility>

// TodoListTool — category "Session control". Giúp agent tự lập kế hoạch cho task
// nhiều bước — agent ghi lại các bước con, đánh dấu bước nào đang làm/đã
// xong, thay vì phải "nhớ" kế hoạch chỉ trong văn bản suy nghĩ tự do.
//
// Khác với MemoryTool (SQLite, tồn tại xuyên suốt nhiều lần chạy agent),
// TodoListTool lưu state trong bộ nhớ (in-memory) vì danh sách việc cần làm
// chỉ có ý nghĩa trong phạm vi MỘT lần agent.run() — mỗi Task của Harness
// tạo ToolRegistry mới nên state tự động được reset sạch giữa các task,
// không cần dọn dẹp thủ công.
//
// Định dạng lệnh (arguments), phân tách bằng " | ":
//   "add | <noi dung>"    -> them 1 task moi, trang thai pending
//   "start | <id>"        -> danh dau task dang lam (in_progress);
//                            tu dong dua moi task in_progress khac ve pending
//                            (dam bao tai 1 thoi diem chi co 1 task dang lam)
//   "done | <id>"         -> danh dau task da hoan thanh
//   "remove | <id>"       -> xoa task khoi danh sach
//   "list" (hoac de trong) -> liet ke toan bo danh sach hien tai

class TodoListTool : public Tool {
public:
    enum class Status { Pending, InProgress, Done };

    struct TodoItem {
        int id;
        std::string content;
        Status status;
    };

private:
    std::vector<TodoItem> items_;
    int nextId_ = 1;

    static std::string trim(const std::string& s);
    static char statusSymbol(Status s);
    std::string renderList() const;
    TodoItem* findById(int id);
    static std::pair<std::string, std::string> splitCommand(const std::string& args);
    static bool parseId(const std::string& s, int& outId);

public:
    TodoListTool() = default;

    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& arguments) override;
};