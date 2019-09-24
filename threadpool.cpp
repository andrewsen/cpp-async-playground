#include "threadpool.h"

static std::shared_ptr<ThreadPool> mainPool{nullptr};
thread_local std::shared_ptr<Looper> ThreadPool::_thisLooper;

ThreadPool::ThreadPool(size_t count, bool addMainLooper)
    : _count{count}, _useMainLooper{addMainLooper}, _isStopped{false} {
    if (count < 1) {
        throw std::runtime_error("Thread pool have to contain at least one thread");
    }

    _pool = new std::thread[_count];

    if (_useMainLooper) {
        ++_count;
    }

    _loopers = new std::shared_ptr<Looper>[_count];
    for (size_t i = 0; i < _count; ++i) {
        _loopers[i] = std::shared_ptr<Looper>(new Looper(static_cast<int>(i), &_taskQueue, _watcher, this));
    }
}

ThreadPool::~ThreadPool() {
    if (!_isStopped) {
        stop();
    }
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

            // Wake up an arbitary thread
            _watcher.notifyOne();
            break;
        case TaskBindingPolicy::BOUND:
            _loopers[policy.boundLooper]->pushBack(task);

            // Wake up all threads (loopers) to be sure that specified looper will execute new task
            _watcher.notifyAll();
            break;
        case TaskBindingPolicy::UNBOUND_EXCEPT: {
            size_t min = SIZE_MAX; //_loopers[0]->getQueueSize();
            std::shared_ptr<Looper>* desired = nullptr;
            for (size_t i = 0; i < _count; ++i) {
                if (_loopers[i]->getQueueSize() < min && static_cast<int>(i) != policy.boundLooper) {
                    desired = &_loopers[i];
                    min = _loopers[i]->getQueueSize();
                }
            }

            if (desired == nullptr) {
                throw std::runtime_error("Can't assign the task to any looper");
            }
            else {
                (*desired)->pushBack(task);
            }
            _watcher.notifyAll();
        }
            break;
    }
    return task;
}

std::shared_ptr<Looper> ThreadPool::getThisLooper() const {
    if (_thisLooper) {
        return _thisLooper;
    }
    throw std::runtime_error("Local looper doesn't exist");
}

void ThreadPool::start() {
    if (_useMainLooper) {
        // Firstly, create _count - 1 threads and start loopers there, then start looper in current thread
        for (size_t i = 1; i < _count; ++i) {
            _pool[i - 1] = std::thread(&ThreadPool::loop, this, i);
        }
        loop(0);
    }
    else {
        for (size_t i = 0; i < _count; ++i) {
            _pool[i] = std::thread(&ThreadPool::loop, this, i);
        }
    }
}

void ThreadPool::stop() {
    if (_isStopped)
        return;

    _isStopped = true;

    for (size_t i = 0; i < _count; ++i) {
        _loopers[i]->stop();
    }

    // Wake up all loopers, so they can do deinit
    _watcher.notifyAll();

    auto threads = _count;
    if (_useMainLooper) {
        --threads;
    }

    for (size_t i = 0; i < threads; ++i) {
        _pool[i].join();
    }

    delete[] _loopers;
    delete[] _pool;
}

void ThreadPool::loop(int id) {
    // Save current looper to thread-local variable

    _thisLooper = _loopers[id];
    bool reload = false;
    do {
        // Start current looper. If exception occurred while processing tasks - report and reload the looper
        try {
            _thisLooper->loop();
            reload = false;
        }
        catch (...) {
            std::cerr << "Exception was risen in looper #" << id << std::endl;
            reload = true;
        }
    } while(reload);
}

void setMainThreadPool(const std::shared_ptr<ThreadPool> &pool) noexcept {
    mainPool = pool;
}

std::shared_ptr<ThreadPool> getMainThreadPool() {
    if (mainPool) {
        return mainPool;
    }
    throw std::runtime_error("Main thread pool is not available");
}
