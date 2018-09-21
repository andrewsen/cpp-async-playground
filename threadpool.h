#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

#include "looper.h"

class ThreadPool
{
    size_t _count {0};
    bool _useMainLooper {false};
    std::thread* _pool {nullptr};
    std::shared_ptr<Looper>* _loopers {nullptr};
    std::atomic_bool _isStopped;
    static thread_local std::shared_ptr<Looper> _thisLooper;
public:
    ThreadPool(size_t count=1, bool addThisThread=false);
    ~ThreadPool();

    void addTask(std::function<void()> task);
    std::shared_ptr<Looper> getThisLooper();

    void start();
    void stop();

private:
    void loop(int id);
};

void setMainThreadPool(std::shared_ptr<ThreadPool> pool);
std::shared_ptr<ThreadPool> getMainThreadPool();

#endif // THREADPOOL_H
