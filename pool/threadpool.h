#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<thread>
#include<assert.h>
#include<future>
#include "safequeue.h"

class ThreadPool {
public:
    explicit ThreadPool(const int n_threads = 4) : threads_(std::vector<std::thread>(n_threads)), isClosed(false) {}
    ThreadPool& operator = (const ThreadPool &) = delete;
    ThreadPool& operator = (ThreadPool &&) = delete;
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ~ThreadPool() {
        shutdown();
    }

    template<typename F, typename ...Args>
    auto submit(F &&f, Args&& ...args) -> std::future<decltype(f(args...))> {
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>> (func);
        std::function<void()> wrap_func = [task_ptr]() {
            (*task_ptr)();
        };
        que_.enqueue(wrap_func);
        cond_lock_.notify_one();
        return task_ptr->get_future();
    };
    
    void init() {
        for(int i = 0; i < (int) threads_.size(); i++) {
            threads_.at(i) = std::thread(ThreadWorker(this, i));
        }
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            isClosed = true;
            cond_lock_.notify_all();
        }
        for(int i = 0; i < threads_.size(); i++) if(threads_.at(i).joinable()) { 
            threads_.at(i).join();
        }
    }
private:
    class ThreadWorker {
    public:
        ThreadWorker(ThreadPool *pool, const int id) : pool_(pool), id_(id) {}
        void operator()() {
            std::function<void()> func;
            bool dequeued;
            while(true) {
                {
                    std::unique_lock<std::mutex> lock(pool_->mtx_); 
                    if(pool_->isClosed) {
                        break;
                    }
                    if(pool_->que_.empty()) {
                        pool_->cond_lock_.wait(lock);
                    }
                    dequeued = pool_->que_.dequeue(func);
                }
                if(dequeued)
                    func();
            }
        }
    private:
        int id_;
        ThreadPool *pool_;
    };

    bool isClosed;
    SafeQueue<std::function<void()>> que_; // task queue
    std::mutex mtx_;
    std::condition_variable cond_lock_;
    std::vector<std::thread> threads_;
};



#endif