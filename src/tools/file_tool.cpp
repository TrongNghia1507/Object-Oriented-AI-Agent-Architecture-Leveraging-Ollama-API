#include "file_tool.h"
#include "../environment/environment.h"

FileTool::FileTool(Environment* env, Mode mode) : env_(env), mode_(mode) {
    if (!env_) {
        throw std::runtime_error("Fatal Error: FileTool can nhan mot con tro Environment hop le!");
    }
}

std::string FileTool::getName() const {
    return mode_ == Mode::Read ? "read_file" : "write_file";
}

std::string FileTool::getDescription() const {
    if (mode_ == Mode::Read) {
        return "Doc noi dung 1 file van ban. Tham so: duong dan file, vi du: 'result.txt'.";
    }
    return "Ghi noi dung vao 1 file van ban (ghi de neu da ton tai). "
           "Tham so bat buoc dung format: 'duong_dan | noi_dung', "
           "vi du: 'result.txt | 255'.";
}

std::string FileTool::execute(const std::string& arguments) {
    if (mode_ == Mode::Read) {
        if (arguments.empty()) return "Loi: thieu duong dan file can doc.";
        return env_->readFile(arguments);
    }

    // Mode::Write — tách "path | content" giống format cua MemorySaveTool
    // ([TAG] | NOI DUNG) de nhat quan trong toan bo project.
    auto sep = arguments.find(" | ");
    if (sep == std::string::npos) {
        return "Loi: sai dinh dang. Can 'duong_dan | noi_dung'.";
    }
    std::string path = arguments.substr(0, sep);
    std::string content = SystemUtils::unescapeContent(arguments.substr(sep + 3));
    return env_->writeFile(path, content);
}

std::unique_ptr<FileTool> makeReadFileTool(Environment* env) {
    return std::make_unique<FileTool>(env, FileTool::Mode::Read);
}

std::unique_ptr<FileTool> makeWriteFileTool(Environment* env) {
    return std::make_unique<FileTool>(env, FileTool::Mode::Write);
}