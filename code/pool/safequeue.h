#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include<queue>
#include<mutex>

template<typename T>
class SafeQueue {
public:
    SafeQueue() = default;
    SafeQueue(SafeQueue &&) = default;
    ~SafeQueue() = default;

    void enqueue(T &item) {
        std::lock_guard<std::mutex> lock(mtx_);
        que_.emplace(item);
    }

    bool dequeue(T &item) {
        std::lock_guard<std::mutex> lock(mtx_);
        if(que_.empty()) {
            return false;
        }
        item = std::move(que_.front());
        que_.pop();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx_);
        return que_.empty();
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return que_.size();
    }

private:
    std::queue<T> que_;
    std::mutex mtx_;
};

#endif