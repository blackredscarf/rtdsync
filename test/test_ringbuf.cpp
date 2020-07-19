#include <rtd/ringbuf.h>
#include <iostream>
#include <thread>

using namespace std;

void test1() {
    auto r = rtd::RingBuffer<int>(6);
    cout << r.Cap() << endl;    // 8
    cout << r.Len() << endl;

    thread([&]() {
        for(int i = 0; i < 11; i++) {
            if(i > 8) {
                this_thread::sleep_for(chrono::seconds(1));
            }
            r.Put(i);
            cout << "Put: " << i << endl;
        }
    }).detach();

    this_thread::sleep_for(chrono::seconds(1));
    for(;;) {
        int v;
        bool res = r.Get(&v, chrono::milliseconds(2000));
        if(!res) { cout << "timeout" << endl; break; }
        cout << "Get: " << v << endl;
    }
}

void test2() {
    auto r = rtd::RingBuffer<int>(6);
    cout << r.Cap() << endl; // 8
    cout << r.Len() << endl;

    thread([&]() {
        int i = 0;
        for(;;) {
            this_thread::sleep_for(chrono::seconds(1));
            if(!r.Put(i)) { break; }   // Break when disposed
            cout << "Put: " << i++ << endl;
        }
    }).detach();

    for(int i = 0; i < 10; i++) {
        int v;
        r.Get(&v);
        cout << "Get: " << v << endl;
    }
    r.Dispose();
    cout << "Disposed" << endl;
}

int main() {
//    test1();
    test2();
}
