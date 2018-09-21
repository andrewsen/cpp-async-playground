#ifndef LOOPER_H
#define LOOPER_H

#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

#include <iostream>

class Looper
{
    typedef std::function<void()> Callback;
    mutable std::mutex _mutex;
    Callback _warmUpCache;
    std::vector<Callback> _queue;
    std::atomic_bool _isStopped;
    const int _index;

public:
    Looper(int index);

    Looper(Looper&& other);

    ~Looper();

    Looper& operator=(Looper&& other) {
        swap(other);
        return *this;
    }

    void pushBack(Callback callback);

    int getIndex() const noexcept;
    size_t getQueueSize() const noexcept;

    void stop() noexcept;

    void loop(bool reentrant);

    void moveTo(int count, std::shared_ptr<Looper> to);
private:
    bool isSafeEmpty();
    void swap(Looper& other);
};

#endif // LOOPER_H
