#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<thread>
#include<assert.h>
#include<future>
#include"safequeue.h"

class ThreadPool {
public:
    explicit ThreadPool(int n_threads) {
        m_threads_.resize(n_threads);
        m_shutdown_ = false;
        for(int i = 0; i < (int)m_threads_.size(); i++) {
            m_threads_.at(i) = std::thread(ThreadWorker(this, i));
        }
    }
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool& operator = (const ThreadPool &) = delete;
    ThreadPool& operator = (ThreadPool &&) = delete;

    template<typename F, typename ...Args> 
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto sptr = std::make_shared<std::packaged_task<decltype(f(args...))()>> (func);

        std::function<void()> wrap_fun = [sptr] {
            (*sptr)();
        };
        que_.enqueue(wrap_fun);
        m_conditional_lock_.notify_one();
        return sptr->get_future();
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(m_conditional_mtx_);
            m_shutdown_ = true;
            m_conditional_lock_.notify_all();
        }
        for(int i = 0; i < (int) m_threads_.size(); i++) if(m_threads_.at(i).joinable()) {
            m_threads_.at(i).join();
        }
    }

private:
    class ThreadWorker {
    public:
        ThreadWorker(ThreadPool* pool, const int id) {
            m_pool_ = pool;
            m_id_ = id;
        }
        void operator()() {
            std::function<void()> func;
            bool dequeued;
            while(true) {
                {
                    std::unique_lock<std::mutex> lock(m_pool_->m_conditional_mtx_);
                    if(m_pool_->m_shutdown_) {
                        break;
                    }
                    if(m_pool_->que_.empty()) {
                        m_pool_->m_conditional_lock_.wait(lock);
                    }
                    dequeued = m_pool_->que_.dequeue(func);
                }
                if(dequeued) {
                    func();
                }
            }
        }
    private:
        int m_id_;
        ThreadPool *m_pool_;
    };
private:
    bool m_shutdown_;
    std::condition_variable m_conditional_lock_;
    SafeQueue<std::function<void()>> que_;
    std::vector<std::thread> m_threads_;
    std::mutex m_conditional_mtx_;
};

#endif