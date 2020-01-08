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
        w->Add(1);
        std::thread([=]() {
            srand(10000000 * i + time(0));
            int s = rand() % 3000 + 200;
            std::this_thread::sleep_for(std::chrono::milliseconds(s));
            ch->Push(i);
            w->Done();
        }).detach();
    }
    w->Wait();
    ch->Close();

    int x;
    while(ch->Pop(&x)) {
        cout << x << endl;
    }
}

int main() {
    TestWait();
}