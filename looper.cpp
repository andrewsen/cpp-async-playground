#include "looper.h"

ThreadPoolBase::~ThreadPoolBase() {
}

Looper::Looper(int index, TaskQueue *queue, QueueWatcher &watcher, ThreadPoolBase* pool)
    : _isStopped{false}, _index{index}, _globalQueue{queue},
      _watcher{watcher}, _pool {pool}, _reschedule {false}, _reschedulePolicy{std::nullopt} {}

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
    std::shared_ptr<Task> task {nullptr};
    while (!_isStopped || !_localQueue.empty()) {
        _watcher.wait([this](){ return !_localQueue.empty() || !_globalQueue->empty() || _isStopped; });
        while (!_localQueue.empty()) {
            task = _localQueue.remove();
            if (task->getState() == TaskState::PENDING) {
                std::cerr << "Looper #" << _index << " took task #" << task->getId() << " from local queue\n";
                task->execute();
                if (_reschedule) {
                    reschedule(task);
                }
            }
        }
        task = _globalQueue->remove();
        if (task) {
            if (task->getState() == TaskState::PENDING) {
                std::cerr << "Looper #" << _index << " took task #" << task->getId() << " from local queue\n";
                task->execute();
                if (_reschedule) {
                    reschedule(task);
                }
            }
        }
    }
}

void Looper::rescheduleCurrentTask() {
    _reschedule = true;
    _reschedulePolicy = std::nullopt;
}

void Looper::rescheduleCurrentTask(const TaskPolicy &policy) {
    _reschedule = true;
    _reschedulePolicy = policy;
}

void Looper::reschedule(const std::shared_ptr<Task> &task) {
    _reschedule = false;
    if(_reschedulePolicy)
        task->setPolicy(*_reschedulePolicy);
    std::cerr << "Task #" << task->getId() << " rescheduled\n";
    _pool->addTask(task);
}

bool Looper::isEmpty() {
    return _localQueue.empty();
}
