#ifndef RTDCHAN_CHAN_H
#define RTDCHAN_CHAN_H

#include <list>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <memory>
#include <vector>
#include <algorithm>

namespace rtd {

using TryState = std::function<int(void)>;

template<typename T>
class chan {
    typedef std::unique_lock<std::mutex> lock;

protected:
    // You cannot create a channel by constructor.
    // Using `MakeChan()` to create a shared_ptr is the best practice.
    chan() : closed_(false) { len_ = 1; }
    explicit chan(int len) : len_(len), closed_(false) {}

public:
    template <typename U>
    friend std::shared_ptr<chan<U>> MakeChan();

    template <typename U>
    friend std::shared_ptr<chan<U>> MakeChan(int len);

    // Push an element into channel.
    // Blocking when channel is filled.
    // Return 1 if success, return 0 if closed.
    int Push(const T& v) {
        lock lc(mu_);
        if (closed_) {
            return 0;
        }
        cv_.wait(lc, [&](){ return q_.size() < len_; });    // blocking if return false
        q_.push_back(v);
        cv_.notify_one();
        return 1;
    }

    // Pop an element from channel.
    // Blocking when channel is empty.
    // Return 1 if success, return 0 if closed and empty.
    int Pop(T* v) {
        lock lc(mu_);
        cv_.wait(lc, [&]() { return closed_ || !q_.empty(); });
        if(q_.empty() && closed_) {
            return 0;
        }
        if (v != nullptr) {
            *v = q_.front();
        }
        q_.pop_front();
        cv_.notify_one();
        return 1;
    }

    // Push an element into channel in non-blocking.
    // Return 1 if success, return 0 if filled, return -1 if closed.
    int TryPush(const T& v) {
        lock lc(mu_);
        if (closed_) {
            return -1;
        }
        if(q_.size() == len_) {
            return 0;
        }
        q_.push_back(v);
        cv_.notify_one();
        return 1;
    }

    // Pop an element from channel in non-blocking.
    // Return 1 if success, return 0 if filled, return -1 if closed.
    int TryPop(T* v) {
        lock lc(mu_);
        if(q_.empty() && closed_) {
            return -1;
        }
        if (q_.empty()) {
            return 0;
        }
        if (v != nullptr) {
            *v = q_.front();
        }
        q_.pop_front();
        cv_.notify_one();
        return 1;
    }

    TryState TryPushState(const T& v) {
        return [=]() -> int {
            return TryPush(v);
        };
    }

    TryState TryPopState(T* v) {
        return [=]() -> int {
            return TryPop(v);
        };
    }

    // Close a channel and cannot Push element any more.
    void Close() {
        lock lc(mu_);
        closed_ = true;
        cv_.notify_all();
    }

    bool IsClosed() {
        return closed_;
    }

private:
    std::list<T> q_;
    std::mutex mu_;
    std::condition_variable cv_;
    bool closed_;
    int len_;
};

template <typename T>
std::shared_ptr<chan<T>> MakeChan() {
    return std::shared_ptr<chan<T>>(new chan<T>());
}

template <typename T>
std::shared_ptr<chan<T>> MakeChan(int len) {
    return std::shared_ptr<chan<T>>(new chan<T>(len));
}

template <typename T>
using SharedChan = std::shared_ptr<chan<T>>;

struct SelectOp {
    int index;
    TryState func;
};

// Listening mutli channels by select.
// select will poll channels to call its TryState function.
// Return a channel index when its TryState function return 1.
// Return -1 when all channels were closed.
// Return -2 when `use_default` is true in one loop if no channel returns.
int Select(const std::initializer_list<TryState> args, bool use_default = false) {
    std::vector<SelectOp> ops;
    int i = 0;
    for(const TryState& f : args) {
        ops.push_back(SelectOp { i++, f});
    }
    std::random_shuffle(ops.begin(), ops.end());

    while(1) {
        int closed_num = 0;
        for(SelectOp& op : ops) {
            int result = op.func();
            if(result == 1) {
                return op.index;
            } else if(result == -1){
                ++closed_num;
            }
        }
        if(closed_num == ops.size()) {
            return -1;
        }
        if(use_default) {
            return -2;
        }
    }
}

}

#endif //RTDCHAN_CHAN_H