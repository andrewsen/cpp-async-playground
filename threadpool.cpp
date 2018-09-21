#include "threadpool.h"

static std::shared_ptr<ThreadPool> mainPool {nullptr};
thread_local std::shared_ptr<Looper> ThreadPool::_thisLooper;

ThreadPool::ThreadPool(size_t count, bool addMainLooper)
    : _count{count}, _useMainLooper{addMainLooper}, _isStopped{false}
{
    if(count < 1)
        return;

    _pool = new std::thread[_count];

    if(_useMainLooper)
        ++_count;

    _loopers = new std::shared_ptr<Looper>[_count];
    for(int i = 0; i < _count; ++i) {
        _loopers[i] = std::shared_ptr<Looper>(new Looper(i));
    }
}

ThreadPool::~ThreadPool() {
    if(!_isStopped)
        stop();
}

void ThreadPool::addTask(std::function<void ()> task) {
    auto minSize {_loopers[0]->getQueueSize()};
    auto maxSize = minSize;
    auto preferred {_loopers[0]};
    auto busiest {_loopers[0]};
    int idx = 0;

    for(int i = 1; i < _count; ++i) {
        auto size = _loopers[i]->getQueueSize();
        if(size < minSize) {
            minSize = size;
            preferred = _loopers[i];
            idx = i;
        }
        else if (size > maxSize) {
            maxSize = size;
            busiest = _loopers[i];
        }
    }

    preferred->pushBack(task);
    std::clog << "Task added to Looper #" << preferred->getIndex() << " (prev. length " << minSize << ")\n";

    minSize++;

    if(maxSize > minSize && (maxSize - minSize) > minSize) {
        std::clog << "Reorganizing queues from longest (" << maxSize << ") to shortest (" << minSize << ")\n";
        int tasksToMove = (maxSize - minSize) / 2;
        if(tasksToMove > 0)
            busiest->moveTo(tasksToMove, preferred);
    }
}

std::shared_ptr<Looper> ThreadPool::getThisLooper() {
    return _thisLooper;
}

void ThreadPool::start() {
    if(_useMainLooper) {
        for(int i = 1; i < _count; ++i) {
            _pool[i-1] = std::thread(&ThreadPool::loop, this, i);
        }
        loop(0);
    }
    else {
        for(int i = 0; i < _count; ++i) {
            _pool[i] = std::thread(&ThreadPool::loop, this, i);
        }
    }
}

void ThreadPool::stop() {
    if(_isStopped)
        return;
    _isStopped = true;

    if(_useMainLooper) {
        for(int i = 1; i < _count; ++i) {
            _loopers[i]->stop();
            _pool[i-1].join();
        }
        _loopers[0]->stop();
    }
    else {
        for(int i = 0; i < _count; ++i) {
            _loopers[i]->stop();
            _pool[i].join();
        }
    }

    delete[] _loopers;
    delete[] _pool;

}

void ThreadPool::loop(int id) {
    //std::clog << "Started Looper #" << id << std::endl;
    _thisLooper = _loopers[id];
    while(!_isStopped) {
        _thisLooper->loop(true);
        if (_loopers[id]->getQueueSize() == 0) {
            int max = 0, idx = 0;
            for(int i = 0; i < _count; ++i) {
                if(i == id)
                    continue;

                int size =_loopers[i]->getQueueSize();
                if(size > max) {
                    max = size;
                    idx = i;
                }
            }

            if(max == 0 || id == idx) {
                std::this_thread::yield();
            }
            else {
                _loopers[idx]->moveTo(max == 1 ? max : (max / 2), _thisLooper);
            }
        }
    }
    //std::clog << "Stopped Looper #" << id << std::endl;
}

void setMainThreadPool(std::shared_ptr<ThreadPool> pool) {
    mainPool = pool;
}

std::shared_ptr<ThreadPool> getMainThreadPool() {
    return mainPool;
}
