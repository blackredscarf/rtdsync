/*

Copyright 2019 flak authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

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
    r.Disposed();
    cout << "Disposed" << endl;
}

int main() {
//    test1();
    test2();
}
