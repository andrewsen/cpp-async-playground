#ifndef LOOPER_H
#define LOOPER_H

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

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

class Looper {
    mutable std::mutex _mutex;
    std::atomic_bool _isStopped;
    const int _index;
    TaskQueue *_globalQueue;
    TaskQueue _localQueue;

public:
    Looper(int index, TaskQueue *queue);

    Looper(Looper &&other);

    ~Looper();

    Looper &operator=(Looper &&other) {
        swap(other);
        return *this;
    }

    void pushBack(const std::shared_ptr<Task> &task);

    int getIndex() const noexcept;
    size_t getQueueSize() const noexcept;

    void stop() noexcept;

    void loop();

    void moveTo(int count, std::shared_ptr<Looper> to);

private:
    bool isEmpty();
    void swap(Looper &other);
    bool canExecuteThis(const std::shared_ptr<Task> &task) const;
};

#endif // LOOPER_H
