#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

// Cấu trúc gói tin giữa các Agent
struct AgentMessage {
    std::string sender;
    std::string receiver;
    std::string content;
};

// Hàng đợi an toàn cho đa luồng (Thread-safe Queue)
class ThreadSafeMessageQueue {
private:
    std::queue<AgentMessage> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

public:
    // Thêm tin nhắn vào hàng đợi
    void push(const AgentMessage& msg) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(msg);
        }
        cv_.notify_one(); // Đánh thức luồng đang chờ
    }

    // Lấy tin nhắn ra khỏi hàng đợi (sẽ block nếu queue trống)
    AgentMessage pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !queue_.empty(); }); 
        AgentMessage msg = queue_.front();
        queue_.pop();
        return msg;
    }
};