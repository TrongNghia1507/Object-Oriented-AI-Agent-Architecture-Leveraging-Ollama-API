#pragma once
#include "tool.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <format>

// DateTimeTool
// Không phụ thuộc Environment/LLMClient — tool "thuần" nhất trong hệ thống,
// nên không cần constructor injection như ExecTool/FileTool/VisionTool.
//
// Kỹ thuật C++20 sử dụng: std::chrono::zoned_time + std::format

class DateTimeTool : public Tool {
private:
    // Chuẩn hoá arguments: bỏ khoảng trắng thừa + hạ chữ thường để so khớp mode.
    static std::string normalize(const std::string& s) {
        std::string out = s;
        out.erase(out.begin(), std::find_if(out.begin(), out.end(),
            [](unsigned char c) { return !std::isspace(c); }));
        out.erase(std::find_if(out.rbegin(), out.rend(),
            [](unsigned char c) { return !std::isspace(c); }).base(), out.end());
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return out;
    }

public:
    std::string getName() const override { return "get_datetime"; }

    std::string getDescription() const override {
        return "Lay ngay/gio hien tai cua he thong. "
               "Tham so (tuy chon, co the de trong): "
               "'full' hoac rong -> ngay+gio day du, "
               "'date' -> chi ngay (YYYY-MM-DD), "
               "'time' -> chi gio (HH:MM:SS), "
               "'unix' -> unix timestamp (so giay tu 1970).";
    }

    std::string execute(const std::string& arguments) override {
        const std::string mode = normalize(arguments);

        const auto now = std::chrono::system_clock::now();

        if (mode == "unix") {
            const auto seconds =
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
            return std::to_string(seconds.count());
        }

        // Quy về độ chính xác giây trước khi gắn timezone để tránh in phần
        // micro/nanosecond thừa trong chuỗi kết quả.
        const auto nowSec = std::chrono::time_point_cast<std::chrono::seconds>(now);
        const std::chrono::zoned_time localTime{std::chrono::current_zone(), nowSec};

        if (mode == "date") {
            return std::format("{:%Y-%m-%d}", localTime);
        }
        if (mode == "time") {
            return std::format("{:%H:%M:%S}", localTime);
        }
        if (mode.empty() || mode == "full" || mode == "now") {
            return std::format("{:%Y-%m-%d %H:%M:%S}", localTime);
        }

        return "Loi: tham so khong hop le. Cac gia tri cho phep: "
               "'full', 'date', 'time', 'unix', hoac de trong.";
    }
};