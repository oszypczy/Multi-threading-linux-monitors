// monitor.h
#pragma once

#include <mutex>
#include <condition_variable>

class Monitor {
public:
    void enter();
    void leave();

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool occupied_ = false;
};