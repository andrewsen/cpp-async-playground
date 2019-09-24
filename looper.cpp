#include "looper.h"

ThreadPoolBase::~ThreadPoolBase() {
}

Looper::Looper(int index, TaskQueue *queue, QueueWatcher &watcher, ThreadPoolBase* pool)
    : _isStopped{false}, _index{index}, _globalQueue{queue},
      _watcher{watcher}, _pool {pool}, _reschedule {false}, _reschedulePolicy{std::nullopt} {}

Looper::~Looper() {
    std::cerr << "Looper destructed\n";
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
        // Looper thread is blocked until any task is scheduled for execution or looper is stopped
        _watcher.wait([this](){ return !_localQueue.empty() || !_globalQueue->empty() || _isStopped; });

        // Firstly, execute all tasks in local queue
        while (!_localQueue.empty()) {
            task = _localQueue.remove();
            if (task->getState() == TaskState::PENDING) {
                std::cerr << "Looper #" << _index << " took task #" << task->getId() << " from local queue\n";
                task->execute();

                // Task can ask looper for rescheduling
                if (_reschedule) {
                    doReschedule(task);
                }
            }
        }
        task = _globalQueue->remove();
        if (task && task->getState() == TaskState::PENDING) {
            std::cerr << "Looper #" << _index << " took task #" << task->getId() << " from global queue\n";
            task->execute();
            if (_reschedule) {
                doReschedule(task);
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

void Looper::doReschedule(const std::shared_ptr<Task> &task) {
    _reschedule = false;

    // If policy wasn't specified in reschedule request previous will be used, otherwise update to new one
    if(_reschedulePolicy) {
        task->setPolicy(*_reschedulePolicy);
    }

    std::cerr << "Task #" << task->getId() << " rescheduled\n";

    // Send task to thread pool
    _pool->addTask(task);
}

bool Looper::isEmpty() {
    return _localQueue.empty();
}
