#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "task.h"

class TaskQueue {
    mutable std::mutex _mutex;
    std::deque<std::shared_ptr<Task>> _queue;
    std::atomic_size_t _size;

public:
    TaskQueue() = default;

    TaskQueue(const TaskQueue &) = delete;

    TaskQueue &operator=(const TaskQueue &) = delete;

    void push(const std::shared_ptr<Task> &task);

    void pop() noexcept;

    std::shared_ptr<Task> remove() noexcept;

    std::shared_ptr<Task> peek() const noexcept;

    // Push, but not lock. Thread-unsafe
    void lpush(const std::shared_ptr<Task> &task);

    // Pop, but not lock. Thread-unsafe
    void lpop() noexcept;

    // Remove, but not lock. Thread-unsafe
    std::shared_ptr<Task> lremove() noexcept;

    // Peek, but not lock. Thread-unsafe
    std::shared_ptr<Task> lpeek() const noexcept;

    bool empty() const noexcept;

    size_t size() const noexcept;

    void clear() noexcept;

    decltype(_queue)::iterator begin() noexcept;

    decltype(_queue)::iterator end() noexcept;

    void lock() const;

    void unlock() const;
};

// Just wrapper above mutex and condition var
struct QueueWatcher {
    std::mutex mutex;
    std::condition_variable cvar;

    void notifyAll() noexcept;

    void notifyOne() noexcept;

    void wait(std::function<bool()> predicate);
};


#endif // TASKQUEUE_H
