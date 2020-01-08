#include <thread>
#include <iostream>
#include <rtd/time.h>
#include <rtd/chan.h>
#include <sstream>

using namespace std;

void TestTimerStop() {
    auto t = rtd::time::Timer(std::chrono::seconds(2));
    t.Start();
    auto ch = t.Channel();

    thread([&t]{
        std::this_thread::sleep_for(std::chrono::seconds(5));
        t.Stop();
        cout << "stop" << endl;
    }).detach();

    std::chrono::system_clock::time_point tp;
    while(ch->Pop(&tp)) {
        cout << rtd::time::Ctime(tp) << endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(8));
}

void TestTimer(int sec, string name) {
    auto t = rtd::time::Timer(std::chrono::seconds(sec));

    stringstream ss;
    ss << name << " " << rtd::time::Ctime(rtd::time::Now()) << endl;
    cout << ss.str() << endl;

    t.Start();
    auto ch = t.Channel();

    std::chrono::system_clock::time_point tp;
    while(ch->Pop(&tp)) {
        stringstream ss;
        ss << name << " " << rtd::time::Ctime(tp) << " end" << endl;
        cout << ss.str() << endl;
    }
}

void TestMultiTimers() {
    vector<tuple<int, string>> v {
        {1, "t1"},
        {5, "t3"},
        {2, "t2"},
        {4, "t4"},
    };
    for(auto t : v) {
        thread([t](){
            TestTimer(get<0>(t), get<1>(t));
        }).detach();
    }
    this_thread::sleep_for(std::chrono::seconds(10));
}

void TestTimeOut() {
    auto t = rtd::time::Timer(std::chrono::seconds(5)).Start();
    auto ch = rtd::MakeChan<bool>();

    thread([ch]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ch->Push(true);
    }).detach();

    switch(rtd::Select({
        t.Channel()->TryPopState(nullptr),
        ch->TryPopState(nullptr)
    })) {
        case 0:
            cout << "Timeout" << endl;
        case 1:
            cout << "Get data" << endl;
    }

}

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

void TestNonBlockingPush() {
    auto t1 = rtd::time::Ticker(std::chrono::seconds(1)).Start();

    stringstream ss;
    ss << rtd::time::Ctime(rtd::time::Now()) << endl;
    cout << ss.str() << endl;

    std::thread([&](){
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::chrono::system_clock::time_point tp;
        while(t1.Channel()->Pop(&tp)) {
            stringstream ss;
            ss << rtd::time::Ctime(tp) << " end" << endl;
            cout << ss.str() << endl;
        }
    }).join();

}

int main() {

//    TestTimer(3, "timer 1");

//    TestNonBlockingPush();
//    TestTimerStop();
    TestTicker();
//    TestTimeOut();

//    TestMultiTimers();

}