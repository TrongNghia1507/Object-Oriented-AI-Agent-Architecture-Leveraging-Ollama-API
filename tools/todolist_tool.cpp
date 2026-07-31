#include "todolist_tool.h"
#include <sstream>
#include <algorithm>
#include <cctype>

std::string TodoListTool::trim(const std::string& s) {
    auto start = std::find_if(s.begin(), s.end(),
        [](unsigned char c) { return !std::isspace(c); });
    auto end = std::find_if(s.rbegin(), s.rend(),
        [](unsigned char c) { return !std::isspace(c); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

char TodoListTool::statusSymbol(Status s) {
    switch (s) {
        case Status::Done:       return 'x';
        case Status::InProgress: return '>';
        default:                 return ' ';
    }
}

std::string TodoListTool::renderList() const {
    if (items_.empty()) {
        return "[TODO LIST] Danh sach dang trong. Dung 'add | <noi dung>' de them task.";
    }
    std::ostringstream out;
    out << "[TODO LIST]\n";
    for (const auto& item : items_) {
        out << "[" << statusSymbol(item.status) << "] #" << item.id
            << " " << item.content << "\n";
    }
    return out.str();
}

TodoListTool::TodoItem* TodoListTool::findById(int id) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [id](const TodoItem& t) { return t.id == id; });
    return it != items_.end() ? &(*it) : nullptr;
}

// Tách "command | rest" thành 2 phần; nếu không có " | " thì rest rỗng.
std::pair<std::string, std::string> TodoListTool::splitCommand(const std::string& args) {
    std::string trimmed = trim(args);
    std::string lower = trimmed;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    size_t sep = trimmed.find('|');
    if (sep == std::string::npos) {
        return { lower, "" };
    }
    std::string cmd = trim(lower.substr(0, sep));
    std::string rest = trim(trimmed.substr(sep + 1));
    return { cmd, rest };
}

bool TodoListTool::parseId(const std::string& s, int& outId) {
    if (s.empty()) return false;
    try {
        size_t consumed = 0;
        outId = std::stoi(s, &consumed);
        return consumed == s.size();
    } catch (...) {
        return false;
    }
}

std::string TodoListTool::getName() const {
    return "todo_list";
}

std::string TodoListTool::getDescription() const {
    return "CHI dung khi task can >= 3 lan goi tool khac nhau hoac co nhanh "
           "dieu kien phuc tap. KHONG dung cho task <= 2 buoc (vd: chi tinh "
           "toan roi tra loi ngay) - hay goi thang tool can thiet, se nhanh hon. "
           "Quan ly danh sach cac task con de tu lap ke hoach nhieu buoc. "
           "Tham so: 'add | noi dung' (them task moi), "
           "'start | id' (danh dau dang lam, chi 1 task in_progress tai 1 thoi diem), "
           "'done | id' (danh dau hoan thanh), "
           "'remove | id' (xoa task), "
           "'list' hoac de trong (xem toan bo danh sach).";
}

std::string TodoListTool::execute(const std::string& arguments) {
    auto [cmd, rest] = splitCommand(arguments);

    if (cmd.empty() || cmd == "list") {
        return renderList();
    }

    if (cmd == "add") {
        if (rest.empty()) {
            return "Loi: thieu noi dung task. Dung dinh dang 'add | noi dung'.";
        }
        items_.push_back({ nextId_, rest, Status::Pending });
        std::string msg = "Da them task #" + std::to_string(nextId_) + ": " + rest;
        ++nextId_;
        return msg;
    }

    if (cmd == "start" || cmd == "done" || cmd == "remove") {
        int id = 0;
        if (!parseId(rest, id)) {
            return "Loi: id khong hop le. Dung dinh dang '" + cmd + " | id' (id la so nguyen).";
        }

        if (cmd == "remove") {
            auto it = std::find_if(items_.begin(), items_.end(),
                [id](const TodoItem& t) { return t.id == id; });
            if (it == items_.end()) {
                return "Loi: khong tim thay task #" + std::to_string(id) + ".";
            }
            items_.erase(it);
            return "Da xoa task #" + std::to_string(id) + ".";
        }

        TodoItem* item = findById(id);
        if (!item) {
            return "Loi: khong tim thay task #" + std::to_string(id) + ".";
        }

        if (cmd == "start") {
            // Đảm bảo tại 1 thời điểm chỉ có tối đa 1 task đang in_progress,
            // giống quy ước của các TodoWrite-style tool phổ biến.
            for (auto& other : items_) {
                if (other.status == Status::InProgress) other.status = Status::Pending;
            }
            item->status = Status::InProgress;
            return "Task #" + std::to_string(id) + " dang duoc thuc hien.";
        }

        // cmd == "done"
        item->status = Status::Done;
        return "Task #" + std::to_string(id) + " da hoan thanh.";
    }

    return "Loi: lenh khong hop le. Cac lenh cho phep: add, start, done, remove, list.";
}