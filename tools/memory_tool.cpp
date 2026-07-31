#include "memory_tool.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <queue>
#include <string_view>
#include <algorithm>
#include <sqlite3.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp> 

using json = nlohmann::json;

// ============================================================================
// 1. QUẢN LÝ CẤU HÌNH (Đọc từ model.env đúng 1 lần - Caching)
// ============================================================================
struct Config {
    int top_k = 5;
    float threshold = 0.55f;
    float sim_weight = 0.8f;
    float recency_weight = 0.2f;
    std::string embedding_model = "nomic-embed-text";
};

class ConfigManager {
private:
    static Config instance_config;
    static bool is_loaded;

    static Config loadConfigFromFile(const std::string& envPath) {
        Config config;
        std::ifstream file(envPath);
        if (!file.is_open()) return config;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream is_line(line);
            std::string key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    if (key == "MEMORY_TOP_K") config.top_k = std::stoi(value);
                    else if (key == "MEMORY_THRESHOLD") config.threshold = std::stof(value);
                    else if (key == "MEMORY_SIM_WEIGHT") config.sim_weight = std::stof(value);
                    else if (key == "MEMORY_RECENCY_WEIGHT") config.recency_weight = std::stof(value);
                    else if (key == "MEMORY_EMBEDDING_MODEL") config.embedding_model = value;
                }
            }
        }
        return config;
    }

public:
    static const Config& getConfig() {
        if (!is_loaded) {
            instance_config = loadConfigFromFile("model.env");
            is_loaded = true;
        }
        return instance_config;
    }
};

// Khởi tạo các biến static
Config ConfigManager::instance_config;
bool ConfigManager::is_loaded = false;

// ============================================================================
// 2. TIỆN ÍCH CHUNG
// ============================================================================
void parseArguments(const std::string& args, std::string& tag, std::string& content) {
    tag = "general"; 
    content = args;
    
    size_t startBracket = args.find('[');
    size_t endBracket = args.find(']');
    size_t sep = args.find('|');
    
    if (startBracket != std::string::npos && endBracket != std::string::npos && startBracket < endBracket) {
        tag = args.substr(startBracket + 1, endBracket - startBracket - 1);
        if (sep != std::string::npos && sep > endBracket) {
            content = args.substr(sep + 1);
        } else {
            content = args.substr(endBracket + 1);
        }
        
        size_t first = content.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) content = content.substr(first);
        size_t last = content.find_last_not_of(" \t\n\r");
        if (last != std::string::npos) content = content.substr(0, last + 1);
    }
}

std::string getCurrentLocalTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
#ifdef _WIN32
    std::tm local_tm;
    localtime_s(&local_tm, &now_time);
    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
#else
    std::tm* local_tm = std::localtime(&now_time);
    std::stringstream ss;
    ss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
#endif
    return ss.str();
}

float calculateRecency(const std::string& created_at) {
    std::tm tm = {};
    std::istringstream ss(created_at);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    std::time_t mem_time = std::mktime(&tm);
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    double diff_seconds = std::difftime(now, mem_time);
    double diff_days = std::max(0.0, diff_seconds / (60 * 60 * 24));
    
    return static_cast<float>(std::exp(-diff_days / 30.0)); 
}

float dot_product(const std::vector<float>& A, const std::vector<float>& B) {
    if (A.size() != B.size() || A.empty()) return 0.0f;
    float dot = 0.0f;
    for (size_t i = 0; i < A.size(); ++i) {
        dot += A[i] * B[i];
    }
    return dot;
}

bool insertMemoryToDB(sqlite3* db, const std::string& tag, const std::string& content, const std::vector<float>& embedding, const std::string& created_at) {
    if (!db) return false;
    const char* sql = "INSERT INTO memories (context_tag, content, embedding, created_at) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, embedding.data(), embedding.size() * sizeof(float), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, created_at.c_str(), -1, SQLITE_TRANSIENT);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

// ============================================================================
// 3. EMBEDDING SERVICE 
// ============================================================================
class EmbeddingService {
private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static void normalize_vector(std::vector<float>& vec) {
        float norm = 0.0f;
        for (float v : vec) norm += v * v;
        if (norm == 0.0f) return;
        norm = std::sqrt(norm);
        for (float& v : vec) v /= norm;
    }

public:
    static std::vector<float> getEmbedding(const std::string& text, const std::string& model_name) {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;
        std::vector<float> embedding;

        curl = curl_easy_init();
        if (!curl) return embedding;

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/embeddings");
        
        json request_body = {
            {"model", model_name},
            {"prompt", text}
        };
        std::string json_str = request_body.dump();

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << "\n";
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                try {
                    json j = json::parse(readBuffer);
                    embedding = j.at("embedding").get<std::vector<float>>();
                    normalize_vector(embedding);
                } catch (const json::exception& e) {
                    std::cerr << "Lỗi phân tích JSON từ Ollama: " << e.what() << "\n";
                }
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return embedding;
    }
};

// ============================================================================
// 4. LỚP QUẢN LÝ DATABASE
// ============================================================================
DatabaseManager::DatabaseManager(const std::string& dbPath) : db(nullptr) {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Lỗi mở DB: " << sqlite3_errmsg(db) << "\n";
        return;
    }
    
    const char* create_table_sql = 
        "CREATE TABLE IF NOT EXISTS memories ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "context_tag TEXT, "
        "content TEXT, "
        "embedding BLOB, "
        "created_at TEXT);";
    
    char* err_msg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Lỗi tạo bảng: " << err_msg << "\n";
        sqlite3_free(err_msg);
    }
}

DatabaseManager::~DatabaseManager() {
    if (db) sqlite3_close(db);
}

// ============================================================================
// 5. TOOL MEMORY SAVE
// ============================================================================
MemorySaveTool::MemorySaveTool(std::shared_ptr<DatabaseManager> dbMgr) : dbManager(dbMgr) {}

std::string MemorySaveTool::execute(const std::string& arguments) {
    if (arguments.empty()) return "Lỗi: Tham số trống.";

    std::string tag, content;
    parseArguments(arguments, tag, content);

    if (content.empty()) return "Lỗi: Không tìm thấy nội dung cần lưu.";

    // Lấy config từ Singleton (đã load sẵn)
    const Config& cfg = ConfigManager::getConfig();
    
    std::vector<float> embedding = EmbeddingService::getEmbedding(content, cfg.embedding_model);
    if (embedding.empty()) return "Lỗi: Không thể lấy embedding (kiểm tra trạng thái Ollama).";

    std::string created_at = getCurrentLocalTime();
    
    // Đảm bảo chỉ dùng getDB()
    bool saved = insertMemoryToDB(dbManager->getDB(), tag, content, embedding, created_at);
    
    if (saved) return "Lưu trí nhớ thành công vào tag: [" + tag + "].";
    return "Lỗi: Thất bại khi ghi vào SQLite.";
}

// ============================================================================
// 6. TOOL MEMORY SEARCH
// ============================================================================
struct SearchResult {
    std::string tag;
    std::string content;
    std::string created_at;
    float similarity;
    float recency;
    float final_score;

    bool operator>(const SearchResult& other) const {
        return final_score > other.final_score;
    }
};

MemorySearchTool::MemorySearchTool(std::shared_ptr<DatabaseManager> dbMgr) : dbManager(dbMgr) {}

std::string MemorySearchTool::execute(const std::string& arguments) {
    if (arguments.empty()) return "Lỗi: Tham số truy vấn trống.";

    std::string tag, query;
    parseArguments(arguments, tag, query);
    
    if (query.empty()) return "Lỗi: Từ khóa tìm kiếm rỗng.";

    // Lấy config từ Singleton (đã load sẵn)
    const Config& cfg = ConfigManager::getConfig();

    std::vector<float> query_embedding = EmbeddingService::getEmbedding(query, cfg.embedding_model);
    if (query_embedding.empty()) return "Lỗi: Không thể phân tích truy vấn từ Ollama.";

    std::priority_queue<SearchResult, std::vector<SearchResult>, std::greater<SearchResult>> top_k_queue;
    
    sqlite3* db = dbManager->getDB(); // Đảm bảo đóng gói qua getDB()
    if (!db) return "Lỗi: Database chưa được khởi tạo.";

    sqlite3_stmt* stmt = nullptr;
    bool is_general_search = (tag.empty() || tag == "general");
    
    // Tách riêng SQL để tối ưu Index và tăng tính rõ ràng
    const char* sql = is_general_search 
        ? "SELECT context_tag, content, embedding, created_at FROM memories;"
        : "SELECT context_tag, content, embedding, created_at FROM memories WHERE context_tag = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (!is_general_search) {
            sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string db_tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            
            // Xử lý an toàn cho Blob Data
            const void* blob_data = sqlite3_column_blob(stmt, 2);
            int blob_bytes = sqlite3_column_bytes(stmt, 2);
            
            if (blob_data == nullptr || blob_bytes == 0) continue; // Bỏ qua nếu bản ghi thiếu embedding

            std::vector<float> db_embedding((const float*)blob_data, ((const float*)blob_data) + (blob_bytes / sizeof(float)));
            
            std::string created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            float similarity = dot_product(query_embedding, db_embedding);
            float recency = calculateRecency(created_at);
            float final_score = (cfg.sim_weight * similarity) + (cfg.recency_weight * recency);

            if (final_score >= cfg.threshold) {
                top_k_queue.push({db_tag, content, created_at, similarity, recency, final_score});
                if (top_k_queue.size() > cfg.top_k) {
                    top_k_queue.pop();
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    if (top_k_queue.empty()) return "Không tìm thấy bộ nhớ nào phù hợp trong ngữ cảnh này.";

    std::vector<SearchResult> results;
    while (!top_k_queue.empty()) {
        results.push_back(top_k_queue.top());
        top_k_queue.pop();
    }
    std::reverse(results.begin(), results.end());

    std::stringstream response;
    response << "Kết quả tìm kiếm (Context: " << tag << "):\n";
    for (const auto& res : results) {
        response << "Tag: [" << res.tag << "] | Điểm: " << std::fixed << std::setprecision(4) << res.final_score 
                 << " | Ngày: " << res.created_at << "\n"
                 << "Nội dung: " << res.content << "\n-----------------\n";
    }

    return response.str();
}