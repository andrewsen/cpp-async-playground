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
        _loopers[i] = std::shared_ptr<Looper>(new Looper(i, &_taskQueue));
    }
}

ThreadPool::~ThreadPool() {
    if (!_isStopped)
        stop();
}

void ThreadPool::addTask(Task *task) {
    addTask(std::shared_ptr<Task>(task));
}

void ThreadPool::addTask(const std::shared_ptr<Task> &task) {
    switch (task->bindingPolicy()) {
        case TaskBindingPolicy::UNBOUND:
            _taskQueue.push(task);
            break;
        case TaskBindingPolicy::BOUND:
            _loopers[task->boundLooperId()]->pushBack(task);
            break;
        case TaskBindingPolicy::UNBOUND_EXCEPT: {
            auto min = _loopers[0]->getQueueSize();
            auto desired = _loopers[0];
            for (int i = 1; i < _count; ++i) {
                if (_loopers[i]->getQueueSize() < min) {
                    desired = _loopers[i];
                    min = _loopers[i]->getQueueSize();
                }
            }
            desired->pushBack(task);
        } break;
    }
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

    if (_useMainLooper) {
        for (int i = 1; i < _count; ++i) {
            _loopers[i]->stop();
            _pool[i - 1].join();
        }
        _loopers[0]->stop();
    } else {
        for (int i = 0; i < _count; ++i) {
            _loopers[i]->stop();
            _pool[i].join();
        }
    }

    delete[] _loopers;
    delete[] _pool;
}

void ThreadPool::loop(int id) {
    // std::clog << "Started Looper #" << id << std::endl;
    _thisLooper = _loopers[id];
    _thisLooper->loop();
    // std::clog << "Stopped Looper #" << id << std::endl;
}

void setMainThreadPool(const std::shared_ptr<ThreadPool> &pool) {
    mainPool = pool;
}

std::shared_ptr<ThreadPool> getMainThreadPool() {
    return mainPool;
}
