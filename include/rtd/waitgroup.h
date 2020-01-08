#ifndef RTDSYNC_WAITGROUP_H
#define RTDSYNC_WAITGROUP_H

#include <stdexcept>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <sstream>
namespace rtd {

class WaitGroup {
protected:
    WaitGroup() : c_(0), w_(0) {}

public:
    friend std::shared_ptr<WaitGroup> MakeWaitGroup();

    // Add 1 before creating a async task.
    void Add(int delta) {
        if(c_ < 0) {
            throw std::logic_error("negative WaitGroup counter");
        }
        c_ += delta;
        std::stringstream ss;
        ss << "c_:" << c_ << "   w_:" << w_ << std::endl;
        std::cout << ss.str();
        if(c_ > 0 || w_ == 0) {
            return;
        }

        for(; w_ != 0; w_--) {
            cv_.notify_one();
        }
    }

    // Done() after complete a async task.
    void Done() {
        Add(-1);
    }

    // Wait() can blocking, until all task being done.
    void Wait() {
        if(c_ == 0) {
            return;
        }
        w_ += 1;
        std::unique_lock lc(mu_);
        cv_.wait(lc, [&]() { return c_ == 0; });
    }

private:
    std::atomic<int> w_; // waiter

    std::atomic<int> c_; // counter

    std::mutex mu_;

    std::condition_variable cv_;
};

std::shared_ptr<WaitGroup> MakeWaitGroup() {
    return std::shared_ptr<WaitGroup>(new WaitGroup());
}

}

#endif //RTDSYNC_WAITGROUP_H
