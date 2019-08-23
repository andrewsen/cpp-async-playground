#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "looper.h"
#include "task.h"

class ThreadPool : ThreadPoolBase {
    size_t _count{0};
    bool _useMainLooper{false};
    std::thread *_pool{nullptr};
    std::shared_ptr<Looper> *_loopers{nullptr};
    std::atomic_bool _isStopped;
    TaskQueue _taskQueue;
    std::mutex _mutex;
    QueueWatcher _watcher;
    static thread_local std::shared_ptr<Looper> _thisLooper;

public:
    ThreadPool(size_t count = 1, bool addThisThread = false);
    virtual ~ThreadPool() override;

    virtual std::shared_ptr<Task> addTask(Task *task) override;
    virtual std::shared_ptr<Task> addTask(const std::shared_ptr<Task> &task) override;
    std::shared_ptr<Looper> getThisLooper();

    void start();
    void stop();

private:
    void loop(int id);
};

void setMainThreadPool(const std::shared_ptr<ThreadPool> &pool);
std::shared_ptr<ThreadPool> getMainThreadPool();

#endif // THREADPOOL_H
