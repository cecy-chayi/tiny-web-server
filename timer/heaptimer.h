#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack callbackFun;
    bool operator < (const TimerNode &t) const {
        return expires < t.expires;
    }
    bool operator > (const TimerNode &t) const {
        return expires > t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer() {
        heap_.reserve(64);
    }
    void adjust(int id, int newExpires);
    void add(int id, int timeout, const TimeoutCallBack& cbFun);
    void doWork(int id);
    void clear();
    void tick();
    void pop();
    int getNextTick();

private:
    void del_(size_t i);
    void shiftup_(size_t i);
    bool shiftdown_(size_t i);
    void swapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    // id: heap_id
    std::unordered_map<int, size_t> ref_;
};

#endif