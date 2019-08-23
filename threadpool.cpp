#include "threadpool.h"

static std::shared_ptr<ThreadPool> mainPool{nullptr};
thread_local std::shared_ptr<Looper> ThreadPool::_thisLooper;

ThreadPool::ThreadPool(size_t count, bool addMainLooper)
    : _count{count}, _useMainLooper{addMainLooper}, _isStopped{false} {
    if (count < 1)
        return;

    _pool = new std::thread[_count];

    if (_useMainLooper)
        ++_count;

    _loopers = new std::shared_ptr<Looper>[_count];
    for (int i = 0; i < _count; ++i) {
        _loopers[i] = std::shared_ptr<Looper>(new Looper(i, &_taskQueue, _watcher, this));
    }
}

ThreadPool::~ThreadPool() {
    if (!_isStopped)
        stop();
}

std::shared_ptr<Task> ThreadPool::addTask(Task *task) {
    return addTask(std::shared_ptr<Task>(task));
}

std::shared_ptr<Task> ThreadPool::addTask(const std::shared_ptr<Task> &task) {
    auto policy = task->getPolicy();
    task->setState(TaskState::PENDING);
    switch (policy.policy) {
        case TaskBindingPolicy::UNBOUND:
            _taskQueue.push(task);
            _watcher.notifyOne();
            break;
        case TaskBindingPolicy::BOUND:
            _loopers[policy.boundLooper]->pushBack(task);
            _watcher.notifyAll();
            break;
        case TaskBindingPolicy::UNBOUND_EXCEPT: {
            size_t min = SIZE_MAX; //_loopers[0]->getQueueSize();
            std::shared_ptr<Looper>* desired = nullptr;
            for (size_t i = 0; i < _count; ++i) {
                if (_loopers[i]->getQueueSize() < min && i != policy.boundLooper) {
                    desired = &_loopers[i];
                    min = _loopers[i]->getQueueSize();
                }
            }
            (*desired)->pushBack(task);
            _watcher.notifyAll();
        } break;
    }
    return task;
}

std::shared_ptr<Looper> ThreadPool::getThisLooper() {
    return _thisLooper;
}

void ThreadPool::start() {
    if (_useMainLooper) {
        for (int i = 1; i < _count; ++i) {
            _pool[i - 1] = std::thread(&ThreadPool::loop, this, i);
        }
        loop(0);
    } else {
        for (int i = 0; i < _count; ++i) {
            _pool[i] = std::thread(&ThreadPool::loop, this, i);
        }
    }
}

void ThreadPool::stop() {
    if (_isStopped)
        return;
    _isStopped = true;

    for (int i = 0; i < _count; ++i) {
        _loopers[i]->stop();
    }

    _watcher.notifyAll();

    auto threads = _count;
    if (_useMainLooper) {
        --threads;
    }

    for (int i = 0; i < threads; ++i) {
        _pool[i].join();
    }

    delete[] _loopers;
    delete[] _pool;
}

void ThreadPool::loop(int id) {
    // std::clog << "Started Looper #" << id << std::endl;
    _thisLooper = _loopers[id];
    bool reload = false;
    do {
        try {
            _thisLooper->loop();
            reload = false;
        } catch (...) {
            std::cerr << "Exception was risen in looper #" << id << std::endl;
            reload = true;
        }
    } while(reload);
    // std::clog << "Stopped Looper #" << id << std::endl;
}

void setMainThreadPool(const std::shared_ptr<ThreadPool> &pool) {
    mainPool = pool;
}

std::shared_ptr<ThreadPool> getMainThreadPool() {
    return mainPool;
}
