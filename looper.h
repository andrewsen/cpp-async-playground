#ifndef LOOPER_H
#define LOOPER_H

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

#include "task.h"

class TaskQueue {
    mutable std::mutex _mutex;
    std::deque<std::shared_ptr<Task>> _queue;
    std::atomic_size_t _size;

public:
    TaskQueue() {}

    TaskQueue(const TaskQueue &) = delete;
    TaskQueue &operator=(const TaskQueue &) = delete;

    void push(const std::shared_ptr<Task> &task) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push_back(task);
        ++_size;
    }

    void pop() {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.pop_front();
        --_size;
    }

    std::shared_ptr<Task> remove() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_size == 0) {
            return {nullptr};
        } else {
            auto task = _queue.front();
            _queue.pop_front();
            --_size;
            return task;
        }
    }

    std::shared_ptr<Task> peek() const noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.front();
    }

    void lpush(const std::shared_ptr<Task> &task) {
        _queue.push_back(task);
        ++_size;
    }

    void lpop() {
        _queue.pop_front();
        --_size;
    }

    std::shared_ptr<Task> lremove() {
        auto task = _queue.front();
        _queue.pop_front();
        --_size;
        return task;
    }

    std::shared_ptr<Task> lpeek() const noexcept {
        return _queue.front();
    }

    bool empty() const noexcept {
        return _size == 0;
    }

    size_t size() const noexcept {
        return _size;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _size = 0;
        _queue.clear();
    }

    decltype(_queue)::iterator begin() {
        return _queue.begin();
    }

    decltype(_queue)::iterator end() {
        return _queue.end();
    }

    void lock() {
        _mutex.lock();
    }

    void unlock() {
        _mutex.unlock();
    }
};

struct QueueWatcher {
    std::mutex mutex;
    std::condition_variable cvar;

    void notifyAll() {
        std::unique_lock<std::mutex> lock(mutex);
        cvar.notify_all();
    }

    void notifyOne() {
        std::unique_lock<std::mutex> lock(mutex);
        cvar.notify_one();
    }

    void wait(std::function<bool()> predicate) {
        std::unique_lock<std::mutex> lock(mutex);
        cvar.wait(lock, predicate);
    }
};

class ThreadPoolBase {
public:
    virtual std::shared_ptr<Task> addTask(Task *task) = 0;
    virtual std::shared_ptr<Task> addTask(const std::shared_ptr<Task> &task) = 0;

    virtual ~ThreadPoolBase();
};

class Looper {
    mutable std::mutex _mutex;
    std::atomic_bool _isStopped;
    const int _index;
    TaskQueue* _globalQueue;
    TaskQueue _localQueue;
    QueueWatcher& _watcher;
    ThreadPoolBase* _pool;
    bool _reschedule;
    std::optional<TaskPolicy> _reschedulePolicy;

public:
    Looper(int index, TaskQueue *queue, QueueWatcher& watcher, ThreadPoolBase* pool);

    ~Looper();

    void pushBack(const std::shared_ptr<Task> &_currentTask);

    int getIndex() const noexcept;
    size_t getQueueSize() const noexcept;

    void stop() noexcept;

    void loop();

    void rescheduleCurrentTask();
    void rescheduleCurrentTask(const TaskPolicy& policy);

private:
    bool isEmpty();
    bool canExecuteThis(const std::shared_ptr<Task> &_currentTask) const;
    void wait();
    void reschedule(const std::shared_ptr<Task> &_currentTask);
};

#endif // LOOPER_H
