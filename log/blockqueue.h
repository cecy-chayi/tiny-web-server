#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include<deque>
#include<condition_variable>
#include<mutex>
#include<sys/time.h>
#include<assert.h>

template<typename T>
class BlockQueue {
public:
    explicit BlockQueue(size_t maxSize = 1000);
    ~BlockQueue();
    bool empty();
    bool full();
    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    bool pop(T &item, int timeout);
    void clear();
    T front(); 
    T back();
    size_t capacity();
    size_t size();

    void flush();
    void close();
private:
    std::deque<T> deq_;
    std::mutex mtx_;
    bool isClose_;
    size_t capacity_;
    std::condition_variable condConsumer_; // consumer condition variable
    std::condition_variable condProducer_; // producer condition variable
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t maxSize) : capacity_(maxSize) {
    assert(maxSize > 0);
    isClose_ = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue() {
    close();
}

template<typename T>
void BlockQueue<T>::close() {
    clear();
    isClose_ = true;
    condConsumer_.notify_all();
    condProducer_.notify_all();
}

template<typename T>
void BlockQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    deq_.clear();
}

template<typename T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> lock(mtx_);
    return deq_.empty();
}

template<typename T>
bool BlockQueue<T>::full() {
    std::lock_guard<std::mutex> lock(mtx_);
    return deq_.size() >= capacity_;
}

template<typename T>
void BlockQueue<T>::push_back(const T& item) {
    // condition_variable usually needs to be used with unique_lock
    std::unique_lock<std::mutex> lock(mtx_);
    while(deq_.size() >= capacity_) {
        condProducer_.wait(lock);
    }
    deq_.push_back(item);
    condConsumer_.notify_one();
}

template<typename T>
void BlockQueue<T>::push_front(const T& item) {
    std::unique_lock<std::mutex> lock(mtx_);
    while(deq_.size() >= capacity_) {
        condProducer_.wait(lock);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template<typename T>
bool BlockQueue<T>::pop(T &item) {
    std::unique_lock<std::mutex> lock(mtx_);
    if(isClose_) {
        return false;
    }
    if(deq_.empty()) {
        condConsumer_.wait(lock);
    }
    item = std::move(deq_.front());
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
bool BlockQueue<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> lock(mtx_);
    if(isClose_) {
        return false;
    }
    if(deq_.empty()) {
        if(condConsumer_.wait_for(lock, std::chrono::seconds(timeout))
            == std::cv_status::timeout) {
                return false;
            }
    }
    item = std::move(deq_.front());
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
T BlockQueue<T>::front() {
    std::unique_lock<std::mutex> lock(mtx_);
    assert(!isClose_ && !deq_.empty());
    return std::move(deq_.front());
}

template<typename T>
T BlockQueue<T>::back() {
    std::unique_lock<std::mutex> lock(mtx_);
    assert(!isClose_ && !deq_.empty());
    return std::move(deq_.back());
}

template<typename T>
size_t BlockQueue<T>::capacity() {
    return capacity_;
}

// flush means sb. should comsume the item
template<typename T>
void BlockQueue<T>::flush() {
    condConsumer_.notify_one();
}
#endif