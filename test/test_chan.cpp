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
        if(ch1->IsClosed() && ch2->IsClosed()) {
            break;
        }
    }
}

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
        if(ch1->IsClosed() && ch2->IsClosed()) {
            break;
        }
    }

}

int main() {
//    TestConsumerProducer();
//    TestMultiChannelsWithSelect();
    TestRandomProducer();

    return 0;
}

