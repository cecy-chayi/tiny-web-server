#include "heaptimer.h"

void HeapTimer::swapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

void HeapTimer::shiftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    int parent = (i - 1) / 2;
    while(parent >= 0) {
        if(heap_[parent] > heap_[i]) {
            swapNode_(i, parent);
            i = parent;
            parent = (i - 1) / 2;
        } else {
            break;
        }
    }
}

// ture: shift down success; false: don't 
bool HeapTimer::shiftdown_(size_t i, size_t n) {
    assert(0 <= i && i < heap_.size());
    assert(0 <= n && n <= heap_.size());
    auto index = i;
    auto child = i * 2 + 1;
    while(child < n) {
        if(child + 1 < n && heap_[child + 1] < heap_[child]) {
            child++;
        }
        if(heap_[child] < heap_[index]) {
            swapNode_(index, child);
            index = child;
            child = 2 * index + 1;
        }
        break;
    }
    return index > i;
}

void HeapTimer::del_(size_t index) {
    assert(0 <= index && index < heap_.size());
    size_t tmp = index;
    size_t n = heap_.size() - 1;
    assert(tmp <= n);
    // if element at the end of queue, 
    // there is no need to move.
    if(index < heap_.size() - 1) {
        swapNode_(tmp, heap_.size() - 1);
        if(!shiftdown_(tmp, n)) {
            shiftup_(tmp);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int newExpires) {
    assert(!heap_.empty() && ref_.count(id));
    heap_[ref_[id]].expires = Clock::now() + MS(newExpires);
    shiftdown_(ref_[id], heap_.size());
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cbFun) {
    assert(id >= 0);
    if(ref_.count(id)) {
        int tmp = ref_[id];
        heap_[tmp].expires = Clock::now() + MS(timeout);
        heap_[tmp].callbackFun = cbFun;
        if(!shiftdown_(tmp, heap_.size())) {
            shiftup_(tmp);
        }
    } else {
        size_t n = heap_.size();
        ref_[id] = n;
        // list initialization
        heap_.push_back({id, Clock::now() + MS(timeout), cbFun});
        shiftup_(n);
    }
}

void HeapTimer::doWork(int id) {
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    auto node = heap_[i];
    node.callbackFun();
    del_(i);
}

void HeapTimer::tick() {
    // clear timeout node
    if(heap_.empty()) {
        return ;
    }
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.callbackFun();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) {
            res = 0;
        }
    }
    return res;
}