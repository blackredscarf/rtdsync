
/*

Timer status machine:
The initial status is `noStatus`,
when timer is pushed into heap, whose status is `waiting`,
when time up and doing task, whose status is `running`.

A timer is stopped, whose status is `deleted`, and waiting the poller to remove it from heap.

A normal timer lifecycle: noStatus -> waiting -> running -> removed
A normal ticker lifecycle: noStatus -> waiting -> running -> waiting -> running -> ...
A stopped timer lifecycle:
 1. noStatus -> removed (not started)
 2. waiting -> deleted -> removed

*/

#ifndef RTDCHAN_TICKER_H
#define RTDCHAN_TICKER_H

#include "chan.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <queue>
#include <mutex>
#include <atomic>

namespace rtd {

namespace time {

using namespace std::chrono;
using SysTimePoint = system_clock::time_point;

SysTimePoint Now() {
    return system_clock::now();
}

// Transit a time_point to ctime string
std::string Ctime(SysTimePoint timePoint) {
    time_t t = system_clock::to_time_t(timePoint);
    std::string cs = std::string(std::ctime(&t));
    cs.erase(cs.size() - 1);
    return cs;
}

// The status of timer
enum class _TimerStatus {
    deleted,
    removed,
    waiting,
    running,
    noStatus
};

// Internal Timer struct
struct _Timer {
    // when is the end of timer
    SysTimePoint when;

    // The duration of ticker
    nanoseconds period;

    // The status of timer
    std::atomic<_TimerStatus> status;

    // What to do reach the `when`.
    // Must be an non-blocking function.
    std::function<void()> Do;

    // What to do reach the end of timer.
    // Must be an non-blocking function.
    std::function<void()> End;
};

using _SharedTimer = std::shared_ptr<_Timer>;

struct _SharedTimerComparsion {
    bool operator()(const _SharedTimer& t1, const _SharedTimer& t2) {
        return t1->when > t2->when;
    }
};

// Timer Heap.
// We put all timers into a minimum heap.
struct _TimersHeap {
    std::priority_queue<_SharedTimer, std::vector<_SharedTimer>, _SharedTimerComparsion> timers;
    std::mutex mu;
    std::condition_variable cv;

    void Pop() {
        if(mu.try_lock()) {
            timers.pop();
            mu.unlock();
        } else {
            timers.pop();
        }
    }

    void Push(const _SharedTimer& t) {
        if(mu.try_lock()) {
            timers.push(t);
            mu.unlock();
        } else {
            timers.push(t);
        }
        cv.notify_one();
    }

    bool Empty() {
        return timers.empty();
    }

    const _SharedTimer& Top() {
        return timers.top();
    }

    void Wait() {
        std::mutex wmu;
        std::unique_lock<std::mutex> lc(wmu);
        cv.wait(lc);
    }

    void WaitUntil(SysTimePoint& until) {
        std::mutex wmu;
        std::unique_lock<std::mutex> lc(wmu);
        cv.wait_until(lc, until);
    }

    void Lock() {
        mu.lock();
    }

    void UnLock() {
        mu.unlock();
    }

} _heap;


void _BadTimer() {
    throw std::logic_error("racy use of timers");
}

// Run a timer.
// Pop a timer and call Do(). Calculate the next `when` if the timer is a ticker, and push into heap again.
// Pop it, call Do() and End() if it is a disposable timer.
void _RunOneTimer(_SharedTimer t, SysTimePoint& now) {
    if(t->period > nanoseconds(0)) {
        auto delta = t->when - now;
        t->when += (1 + -delta/t->period) * t->period;
        _heap.Pop();
        _heap.Push(t);
        t->status = _TimerStatus::waiting;

        _heap.UnLock();
        t->Do();
        _heap.Lock();

    } else {    // disposable timer when period == 0
        _heap.Pop();

        _heap.UnLock();
        t->Do();
        t->End();
        _heap.Lock();

        t->status = _TimerStatus::removed;
    }
}

// Check the top timer of the heap.
// Return -1 if heap empty.
// Return 1 if run a timer successfully.
// Return 0 if do not reach the `when`.
int _RunTimer(SysTimePoint* until) {
    while(1) {
        if(_heap.Empty()) {
            return -1;
        }
        _SharedTimer t = _heap.Top();
        auto now = Now();
        if(t->status == _TimerStatus::waiting){
            if(t->when > now) {
                *until = t->when;
                return 0;
            }
            t->status = _TimerStatus::running;
            _RunOneTimer(t, now);
            return 1;

        } else if(t->status == _TimerStatus::deleted) {
            _heap.Pop();
            t->status = _TimerStatus::removed;
            if(_heap.Empty()) {
                return -1;
            }
            break;

        } else if(t->status == _TimerStatus::noStatus
                    || t->status == _TimerStatus::removed) {    // An inactive timer should be popped
            _BadTimer();

        } else if(t->status == _TimerStatus::running) {   // Here should be locked
            _BadTimer();

        } else {
            _BadTimer();
        }
    }
    return 0;
}

// Timers poll.
// Blocking until the next `when`, unless a new timer push into heap.
// Blocking if heap empty until a new timer push into heap.
void _TimersPoll() {
    SysTimePoint until;
    while(1) {
        _heap.Lock();
        int res = _RunTimer(&until);
        _heap.UnLock();
        if(res == 0) {
            _heap.WaitUntil(until);
        } else if (res == -1) {         // No timer now
            _heap.Wait();   // Waiting a new timer notice
        }
    }
}

// Run the timers poll in a thread.
void _RunTimersPoll() {
    std::thread([]() {
        _TimersPoll();
    }).detach();
}

// Running the timers poll since the beginning of program
struct _TimersPollRunner {
    _TimersPollRunner() {
        _RunTimersPoll();
    }
} _run;

// Clean timer heap.
// Pop the timer from heap that have been deleted.
bool _CleanTimer() {
    if(_heap.Empty()) {
        return true;
    }
    while(1){
        _SharedTimer t = _heap.Top();
        if(t->status == _TimerStatus::deleted) {
            _heap.Pop();
            t->status = _TimerStatus::removed;
        } else {
            return true;
        }
    }
}

// Add a timer.
// Clean timer heap before adding.
void _AddTimer(const _SharedTimer& t) {
    if(t->status != _TimerStatus::noStatus) {
        _BadTimer();
    }
    t->status = _TimerStatus::waiting;
    _heap.Lock();
    if(!_CleanTimer()) {
        _BadTimer();
    }
    _heap.Push(t);
    _heap.UnLock();
}

// Stop a timer.
// Delete a timer by signing the status as deleted.
// _CleanTimer() and timers poll will pop the deleted timers.
// The function will blocking if the timer to delete is running.
// Return false if the timer is stopped or stopping.
bool _StopTimer(const _SharedTimer& t) {
    while(1) {
        if(t->status == _TimerStatus::waiting) {
            t->status = _TimerStatus::deleted;
            return true;

        } else if(t->status == _TimerStatus::deleted
                  || t->status == _TimerStatus::removed) {
            return false;

        } else if(t->status == _TimerStatus::running) {
            continue; // try again later

        } else if(t->status == _TimerStatus::noStatus) { // do not start or end of run
            t->status = _TimerStatus::removed;
            return true;

        } else {
            _BadTimer();

        }
    }
}

template <typename T, typename U>
SysTimePoint When(duration<T, U> d) {
    return Now() + d;
}

// The user interface of timer.
template <typename T, typename U>
class Timer {
public:
    explicit Timer(duration<T, U> period) : period_(period), t_(std::make_shared<_Timer>()) {
        t_->status = _TimerStatus::noStatus;
    }

    // Start the timer.
    Timer& Start() {
        if(t_->status != _TimerStatus::noStatus) {
            throw std::logic_error("cannot start timer that has been started or stopped.");
        }
        _Set();
        _AddTimer(t_);
        return *this;
    }

    // Stop the timer and close the channel.
    // You cannot restart the timer after Stop().
    bool Stop() {
        bool ok = _StopTimer(t_);
        if(ok) {
            c_->Close();
        }
        return ok;
    }

    // Return a channel.
    // The channel will be pushed a now time when time up.
    SharedChan<SysTimePoint> Channel() {
        return c_;
    }

    bool isStop() {
        return t_->status == _TimerStatus::removed
            || t_->status == _TimerStatus::deleted;
    }

protected:
    virtual void _Set() {
        c_ = MakeChan<SysTimePoint>();
        t_->period = nanoseconds(0);
        t_->when = When(period_);
        t_->status = _TimerStatus::noStatus;
        t_->Do = [&]() {
            c_->TryPush(Now());     // non-blocking push
        };
        t_->End = [&]() {   // close the channel in the end of timer
            c_->Close();
        };
    }

protected:
    SharedChan<SysTimePoint> c_;

    _SharedTimer t_;

    duration<T, U> period_;
};

// A ticker.
template <typename T, typename U>
class Ticker : public Timer<T, U> {

public:
    explicit Ticker(const duration<T, U>& period) : Timer<T, U>(period) {

    }

protected:
    using Timer<T, U>::c_;
    using Timer<T, U>::t_;
    using Timer<T, U>::period_;

    void _Set() override {
        c_ = MakeChan<SysTimePoint>();
        t_->period = nanoseconds(period_);
        t_->when = When(period_);
        t_->status = _TimerStatus::noStatus;
        t_->Do = [&]() {
            c_->TryPush(Now());
        };
        t_->End = [&]() {
            c_->Close();
        };
    }
};

} }

#endif //RTDCHAN_TICKER_H
