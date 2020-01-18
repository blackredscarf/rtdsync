# Rtd Sync
A C++ library implements channel, timer, wait group, for multi-thread synchronization, inspired by Golang design.

Included Components:
- Chan: Channel implementation.
- Timer: A timer returning a channel.
- WaitGroup: Blocking until all tasks being done.

## Install

Environment:
- C++ 11+
- CMake 3.13+

Commands:
```
git clone https://github.com/blackredscarf/rtdsync
cd rtdsync
mkdir build
cd build
cmake ..
cmake build . --target install
```

## Example

### Channel

#### Consumer & Producer

```cpp
#include <rtd/chan.h>
#include <thread>
#include <iostream>
using namespace std;

void TestConsumerProducer() {
    auto ch1 = rtd::MakeChan<int>(3); // buffered channel

    // producer
    std::thread([ch1]() {
        for(int i = 0; i < 5; i++) {
            ch1->Push(i); // blocking when buffer filled
            cout << "ch1 push: " << i << endl;
        }
        ch1->Close();
    }).detach();

    // consumer
    while(1){
        int x;
        // blocking when empty
        // return 0 if closed and empty
        if(ch1->Pop(&x)) {
            cout << "ch1 pop: " << x << endl;
            this_thread::sleep_for(chrono::milliseconds(1000));
        } else {
            break;
        }
    }
}

int main() {

    TestConsumerProducer();

}
```

#### Select
```cpp
void TestMultiChannelsWithSelect() {
    auto ch1 = rtd::MakeChan<int>(); // unbuffered, size=1
    auto ch2 = rtd::MakeChan<int>();

    std::thread([ch1]() {
        for(int i = 0; i < 5; i++) {
            ch1->Push(i);
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
        ch1->Close();
    }).detach();

    std::thread([ch2]() {
        for(int i = 0; i < 5; i++) {
            ch2->Push(i);
            this_thread::sleep_for(chrono::milliseconds(500));
        }
        ch2->Close();
    }).detach();

    while(1){
        int x;
        switch (rtd::Select({
                                ch1->TryPopState(&x),
                                ch2->TryPopState(&x)
                            })) {
            case 0:
                cout << "ch1: " << x << endl;
                break;
            case 1:
                cout << "ch2: " << x << endl;
                break;
            case -1:
                cout << "ch1 and ch2 closed" << endl;
                break;
        }
        if(ch1->isClosed() && ch2->isClosed()) {
            break;
        }
    }
}
```

#### Random Producer
```cpp
void TestRandomProducer() {
    auto ch1 = rtd::MakeChan<int>(3);
    auto ch2 = rtd::MakeChan<int>(3);

    std::thread([ch1]() {
        for(int i = 0; i < 5; i++) {
            this_thread::sleep_for(chrono::milliseconds(1200));
            int x;
            ch1->Pop(&x);
            cout << "ch1: " << x << endl;
        }
        ch1->Close();
    }).detach();

    std::thread([ch2]() {
        for(int i = 0; i < 5; i++) {
            this_thread::sleep_for(chrono::milliseconds(300));
            int x;
            ch2->Pop(&x);
            cout << "ch2: " << x << endl;
        }
        ch2->Close();
    }).detach();

    for(int i = 0; i < 1000; i++) {
        switch (rtd::Select({
                                ch1->TryPushState(i),
                                ch2->TryPushState(i)
                            })) {
            case 0:
                cout << "ch1 push: " << i << endl;
                break;
            case 1:
                cout << "ch2 push: " << i << endl;
                break;
            case -1:
                cout << "ch1 and ch2 closed" << endl;
                break;
        }
        this_thread::sleep_for(chrono::milliseconds(100));
        if(ch1->isClosed() && ch2->isClosed()) {
            break;
        }
    }

}
```

### Time
#### Timer
```cpp
#include <thread>
#include <iostream>
#include <rtd/time.h>
#include <rtd/chan.h>
#include <sstream>
using namespace std;

void TestTimer(int sec, string name) {
    auto t = rtd::time::Timer(std::chrono::seconds(sec));

    stringstream ss;
    ss << name << " " << rtd::time::Ctime(rtd::time::Now()) << endl;
    cout << ss.str() << endl;

    t.Start();  // start timer
    auto ch = t.Channel();  // return a channel

    std::chrono::system_clock::time_point tp;
    while(ch->Pop(&tp)) {   // listen the channel
        stringstream ss;
        ss << name << " " << rtd::time::Ctime(tp) << " end" << endl;
        cout << ss.str() << endl;
    }
}

int main() {
    TestTimer(3, "timer 1");
}
```

#### Ticker
```cpp
void TestTicker() {
    auto t1 = rtd::time::Ticker(std::chrono::seconds(1)).Start();

    thread([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        t1.Stop();
    }).detach();

    std::chrono::system_clock::time_point tp;
    while(t1.Channel()->Pop(&tp)) {
        stringstream ss;
        ss << rtd::time::Ctime(tp) << " end" << endl;
        cout << ss.str() << endl;
    }
}
```

### WaitGroup
```cpp
#include <rtd/waitgroup.h>
#include <rtd/chan.h>
#include <iostream>
#include <thread>
#include <ctime>
using namespace std;

void TestWait() {
    auto w = rtd::MakeWaitGroup();
    auto ch = rtd::MakeChan<int>(10);
    for(int i = 0; i < 5; i++) {
        w->Add(1);  // add a task
        std::thread([=]() {
            srand(10000000 * i + time(0));
            int s = rand() % 3000 + 200;
            std::this_thread::sleep_for(std::chrono::milliseconds(s));
            ch->Push(i);    // put some messages in channel
            w->Done();      // a task is done
        }).detach();
    }
    w->Wait();  // blocking until all tasks being done.
    ch->Close();

    int x;
    while(ch->Pop(&x)) {    // get messages from channel
        cout << x << endl;
    }
}

int main() {
    TestWait();
}
```