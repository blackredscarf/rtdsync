#ifndef RTDSYNC_RINGBUF_H
#define RTDSYNC_RINGBUF_H

#include <cstdio>
#include <atomic>
#include <chrono>
#include <stdexcept>

namespace rtd {

using namespace std::chrono;
using SysTimePoint = system_clock::time_point;

template <typename T>
struct _node {
    std::atomic<size_t> pos;
    T data;
    _node() {}
    explicit _node(size_t position): pos(position) {}
};

size_t roundUp(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

template <typename T>
class RingBuffer {
public:
    RingBuffer(size_t size):cap_(roundUp(size)) {
        buf_ = new _node<T>[cap_];
        for(size_t i = 0; i < cap_; i++) {
            buf_[i].pos = i;
        }
        mask_ = cap_ - 1;
        dequeue_ = 0;
        queue_ = 0;
        disposed_ = false;
    }

    bool Put(T v) {
        _node<T>* n;
        size_t pos = queue_;
        for(;;) {
            if(disposed_) {
                return false;
            }

            n = &buf_[pos&mask_];
            size_t seq = n->pos;
            size_t diff = seq - pos;
            if(diff == 0) {
                if (queue_.compare_exchange_weak(pos, pos + 1)) {
                    break;
                }
            } else if(diff < 0) {
                throw std::runtime_error("Putting operation in compromised state.");
            } else {
                pos = queue_;
            }
        }

        n->data = v;
        n->pos = pos + 1;
        return true;
    }

    bool Get(T* data, milliseconds timeout=milliseconds(0)) {
        _node<T>* n;
        size_t pos = dequeue_;
        SysTimePoint start = system_clock::now();

        for(;;) {
            if(disposed_) {
                return false;
            }

            n = &buf_[pos&mask_];
            size_t seq = n->pos;
            size_t diff = seq - (pos + 1);
            if(diff == 0) {
                if(dequeue_.compare_exchange_weak(pos, pos + 1)) {
                    break;
                }
            } else if(diff < 0) {
                throw std::runtime_error("Getting operation in compromised state.");
            } else {
                pos = dequeue_;
            }

            auto end = system_clock::now();
            if(timeout > milliseconds(0) && timeout <= duration_cast<milliseconds>(end - start)){
                return false;
            }
        }
        *data = n->data;
        n->pos = pos + mask_ + 1;
        return true;
    }

    void Dispose() {
        disposed_ = true;
    }

    bool IsDisposed() {
        return disposed_;
    }

    size_t Len() {
        return queue_ - dequeue_;
    }

    size_t Cap() {
        return cap_;
    }

private:
    _node<T>* buf_;
    size_t cap_;
    size_t mask_;
    std::atomic<bool> disposed_;
    std::atomic<size_t> queue_;
    std::atomic<size_t> dequeue_;
};
}

#endif //RTDSYNC_RINGBUF_H
