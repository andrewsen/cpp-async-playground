#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

#include "application.h"
#include "promise.h"

using namespace std;

template <class... Args> class Event {
    typedef void (*Callback)(Args...);

protected:
    mutable std::mutex _mutex;
    std::list<Callback> _handlers;

public:
    Event() {}

    Event(const Event &other) {
        std::lock_guard<std::mutex> lock(other._mutex);
        _handlers = other._handlers;
    }

    virtual ~Event() {}

    Event &operator=(const Event &other) {
        std::lock_guard<std::mutex> lock(_mutex);
        _handlers.clear();

        other._mutex.lock();
        _handlers = other._handlers;
        other._mutex.unlock();
        return *this;
    }

    virtual void invoke(Args... args) {
        std::lock_guard<std::mutex> locker(_mutex);
        for (auto handler : _handlers)
            handler(args...);
    }

    void operator+=(Callback callback) {
        std::lock_guard<std::mutex> locker(_mutex);
        _handlers.push_back(callback);
    }

    void operator-=(Callback callback) {
        std::lock_guard<std::mutex> locker(_mutex);
        _handlers.remove(callback);
    }

    void operator()(Args... args) {
        invoke(args...);
    }

protected:
    void swap(Event &other) {
        std::lock(_mutex, other._mutex);
        std::lock_guard<std::mutex> lock_a(_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock_b(other._mutex, std::adopt_lock);

        std::swap(_handlers, other._handlers);
    }
};

template <class... Args> class AsyncEvent : public Event<Args...> {
public:
    virtual void invoke(Args... args) override {
        auto pool = getMainThreadPool();

        std::lock_guard<std::mutex> locker(this->_mutex);
        for (auto handler : this->_handlers) {
            // pool->addTask([handler, args...]() {
            //    handler(args...);
            //});
        }
    }
};

template <class T> class List {
private:
    vector<T> vec;

public:
    // typedef bool(*SelectorCb)(const T& elem);
    typedef std::function<bool(const T &elem)> SelectorCb;

    AsyncEvent<T> addedEvent;
    AsyncEvent<T> removedEvent;

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

    Promise<List<T>> select(SelectorCb selector) {
        return Promise<List<T>>([this, selector]() {
            List<T> result;
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
        // cout << "a, b: " << a << ", " << b << endl;
        std::this_thread::sleep_for(100ms);
        return a * b;
    }, a, b);
}

template <class T> void pfor(T start, T end, std::function<void(T)> action) {
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
        cerr << "\tDummy(int x)\n";
    }

    Dummy(const Dummy& other)
        : x{other.x} {
        cerr << "\tDummy(const Dummy& other)\n";
    }

    Dummy(const Dummy&& other)
        : x{other.x} {
        cerr << "\tDummy(const Dummy&& other)\n";
    }

    Dummy& operator=(const Dummy& other) {
        x = other.x;
        cerr << "\tDummy& operator=(const Dummy& other)\n";
        return *this;
    }

    Dummy& operator=(const Dummy&& other) {
        x = other.x;
        cerr << "\tDummy& operator=(const Dummy&& other)\n";
        return *this;
    }

    ~Dummy() {
        cerr << "\t~Dummy()\n";
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

int main() {
    auto app = Application::create();
    // ThreadPool pool{4};
    // setMainThreadPool(&pool);

    List<int> numbers;
    numbers.addedEvent += [](int num) {
        cout << "\tAdded number " << num << " | " << (getMainThreadPool()->getThisLooper()->getIndex()) << endl;
    };
    numbers.addedEvent += [](int num) {
        // cout << "Negative " << -num << endl;
    };

    // for(int i = 0; i < 100; ++i)
    //    numbers.pushBack(i);

    /*app->addTask([]() {
        pfor<int>(0, 100, [](int i) {
            // std::this_thread::sleep_for(100ms);
            std::this_thread::sleep_for(10ms + std::chrono::milliseconds(10 + rand() % 300));
            clog << i << " on " << getMainThreadPool()->getThisLooper()->getIndex() << endl;
        });

        cin.get();
        clog << "Exiting\n";
        Application::getInstance()->exit(0);
    });*/

    //calc(2, 4)
    //    .then([](int res) {
    //        cerr << "Result is: " << res << std::endl;
    //    });

    //Dummy d{42};
    Promise<Dummy>(&dummyFun, 42)
            .then([](Dummy d) {
                std::cerr << "Dummy.x = " << d.getX(0) << std::endl;
                Application::getInstance()->exit(0);
            });

    /*for(int i = 0; i < 15; ++i) {
        //numbers.pushBack(i*i);
        calc(i, (i/2) - (i/3))
            .then([](int res){
                cout << "Result: " << res << endl;
            });
    }*/
    /*for(int i = 0; i < 10; ++i) {
        numbers.select([i](const int& elem) -> bool {
            std::this_thread::sleep_for(10ms);
            return elem % (i+1) == 0;
        }).then([i](List<int>& results) {
            clog << "elem % " << i+1 << ": " << results.size();
            //for(int i = 0; i < results.size(); ++i)
            //    clog << results[i] << " ";
            clog << endl;
        });
    }*/

    return app->exec();
}
