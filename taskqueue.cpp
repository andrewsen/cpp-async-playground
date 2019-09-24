#include "taskqueue.h"

void TaskQueue::push(const std::shared_ptr<Task> &task) {
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.push_back(task);
    ++_size;
}

void TaskQueue::pop() noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.pop_front();
    --_size;
}

std::shared_ptr<Task> TaskQueue::remove() noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_size == 0) {
        return {nullptr};
    }
    else {
        auto task = _queue.front();
        _queue.pop_front();
        --_size;
        return task;
    }
}

std::shared_ptr<Task> TaskQueue::peek() const noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    return _queue.front();
}

void TaskQueue::lpush(const std::shared_ptr<Task> &task) {
    _queue.push_back(task);
    ++_size;
}

void TaskQueue::lpop() noexcept {
    _queue.pop_front();
    --_size;
}

std::shared_ptr<Task> TaskQueue::lremove() noexcept {
    auto task = _queue.front();
    _queue.pop_front();
    --_size;
    return task;
}

std::shared_ptr<Task> TaskQueue::lpeek() const noexcept {
    return _queue.front();
}

bool TaskQueue::empty() const noexcept {
    return _size == 0;
}

size_t TaskQueue::size() const noexcept {
    return _size;
}

void TaskQueue::clear() noexcept {
    std::lock_guard<std::mutex> lock(_mutex);
    _size = 0;
    _queue.clear();
}

decltype(TaskQueue::_queue)::iterator TaskQueue::begin() noexcept {
    return _queue.begin();
}

decltype(TaskQueue::_queue)::iterator TaskQueue::end() noexcept {
    return _queue.end();
}

void TaskQueue::lock() const {
    _mutex.lock();
}

void TaskQueue::unlock() const {
    _mutex.unlock();
}

void QueueWatcher::notifyAll() noexcept {
    std::unique_lock<std::mutex> lock(mutex);
    cvar.notify_all();
}

void QueueWatcher::notifyOne() noexcept {
    std::unique_lock<std::mutex> lock(mutex);
    cvar.notify_one();
}

void QueueWatcher::wait(std::function<bool ()> predicate) {
    std::unique_lock<std::mutex> lock(mutex);
    cvar.wait(lock, predicate);
}
