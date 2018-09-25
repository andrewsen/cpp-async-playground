#include "looper.h"

Looper::Looper(int index, TaskQueue *queue) : _isStopped{false}, _index{index}, _globalQueue{queue} {}

Looper::Looper(Looper &&other) : _index{other._index} {
    swap(other);
}

Looper::~Looper() {
    std::clog << "~Looper destructed\n";
    _isStopped = true;
    if (!_localQueue.empty())
        _localQueue.clear();
}

void Looper::pushBack(const std::shared_ptr<Task> &task) {
    std::lock_guard<std::mutex> guard(_mutex);
    _localQueue.push(task);
}

size_t Looper::getQueueSize() const noexcept {
    return _localQueue.size();
}

int Looper::getIndex() const noexcept {
    return _index;
}

void Looper::stop() noexcept {
    _isStopped = true;
}

void Looper::loop() {
    std::shared_ptr<Task> task{nullptr};
    while (!_isStopped || !_localQueue.empty()) {
        while (!_localQueue.empty()) {
            std::cerr << "Looper #" << _index << " took task from local queue\n";
            task = _localQueue.remove();
            task->execute();
        }
        task = _globalQueue->remove();
        if (task) {
            std::cerr << "Looper #" << _index << " took task from global queue\n";
            task->execute();
        }
    }
}

bool Looper::isEmpty() {
    return _localQueue.empty();
}

void Looper::swap(Looper &other) {
    std::lock(_mutex, other._mutex);
    std::lock_guard<std::mutex> lock_a(_mutex, std::adopt_lock);
    std::lock_guard<std::mutex> lock_b(other._mutex, std::adopt_lock);

    // std::swap(_localQueue, other._localQueue);
    std::swap(_globalQueue, other._globalQueue);
    _isStopped.exchange(other._isStopped);
    // std::swap(_isStopped, other._isStopped);
}
