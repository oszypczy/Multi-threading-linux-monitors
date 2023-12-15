// monitor.cpp
#include "monitor.h"

void Monitor::enter() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (occupied_) {
        condition_.wait(lock);
    }
    occupied_ = true;
}

void Monitor::leave() {
    std::unique_lock<std::mutex> lock(mutex_);
    occupied_ = false;
    condition_.notify_one();
}
