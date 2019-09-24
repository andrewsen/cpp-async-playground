#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

#include "application.h"
#include "promise.h"
#include "event.h"

using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""s;

template <class T>
class DummyList {
private:
    std::vector<T> vec;

public:
    using Selector = std::function<bool(const T &elem)>;

    Event<DummyList, T> addedEvent;
    Event<DummyList, T> removedEvent;

    void pushBack(const T &val) {
        vec.push_back(val);
        addedEvent(vec.at(vec.size() - 1));
    }

    void pushBack(T &&val) {
        vec.push_back(val);
        addedEvent(vec.at(vec.size() - 1));
    }

    void popBack() {
        removedEvent(vec[vec.size() - 1]);
        vec.pop_back();
    }

    T operator[](int idx) const {
        return vec[idx];
    }

    T &operator[](int idx) {
        return vec[idx];
    }

    int size() const {
        return vec.size();
    }

    Promise<DummyList<T>> select(Selector selector) {
        return Promise<DummyList<T>>([this, selector]() {
            DummyList<T> result;
            for (auto &elem : vec) {
                if (selector(elem))
                    result.pushBack(elem);
            }
            return result;
        });
    }
};

Promise<int> calc(int a, int b) {
    return Promise<int>([](int a, int b) {
        std::this_thread::sleep_for(100ms);
        return a * b;
    }, a, b);
}

template <class T>
void pfor(T start, T end, std::function<void(T)> action) {
    auto inst = Application::getInstance();
    for (T i = start; i < end; ++i) {
        std::this_thread::sleep_for(40ms);
        inst->addTask([i, action]() { action(i); });
    }
}

class Dummy {
    int x;

public:
    Dummy(int x)
        : x{x} {
        std::cerr << "\tDummy(int x)\n";
    }

    Dummy(const Dummy& other)
        : x{other.x} {
        std::cerr << "\tDummy(const Dummy& other)\n";
    }

    Dummy(const Dummy&& other)
        : x{other.x} {
        std::cerr << "\tDummy(const Dummy&& other)\n";
    }

    Dummy& operator=(const Dummy& other) {
        x = other.x;
        std::cerr << "\tDummy& operator=(const Dummy& other)\n";
        return *this;
    }

    Dummy& operator=(const Dummy&& other) {
        x = other.x;
        std::cerr << "\tDummy& operator=(const Dummy&& other)\n";
        return *this;
    }

    ~Dummy() {
        std::cerr << "\t~Dummy()\n";
    }

    int getX(int y) const { return x + y; }
    void setX(int value) { x = value; }
};

int funDummy(Dummy d) {
    std::this_thread::sleep_for(1s);
    return d.getX(24);
}

Dummy dummyFun(int x) {
    std::this_thread::sleep_for(1s);
    return Dummy {x};
}

void doWork(int x) {
    std::this_thread::sleep_for(2s);
    std::cerr << "Slept\n";
}

int main() {
    auto app = Application::create();

    DummyList<int> numbers;
    numbers.addedEvent += async_event [](int num) {
        std::this_thread::sleep_for(10ms);
        std::cerr << "\tAdded number " << num << " | " << (getMainThreadPool()->getThisLooper()->getIndex()) << std::endl;
    };
    numbers.addedEvent += async_event [](int num) {
        std::cerr << "Negative " << -num << std::endl;
    };

    for(int i = 0; i < 15; ++i) {
        numbers.pushBack(i*i);
    }

    app->add([]() {
        static int count = 0;
        std::cerr << "Task, count = " << count << "\n";
        if(count < 10) {
            std::this_thread::sleep_for(100ms);
            App->rescheduleTask(TaskPolicy {
                                   TaskBindingPolicy::UNBOUND_EXCEPT,
                                   App->getThreadId()
                               });
        }
        else {
            App->exit(0);
        }
        ++count;
    });

    return app->exec();
}
