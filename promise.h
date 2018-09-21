#ifndef PROMISE_H
#define PROMISE_H

#include <functional>
#include <condition_variable>
#include <experimental/optional>
#include "threadpool.h"

template<class T>
class PromiseTask {
    std::function<T()> _target;
    std::function<void (T&)> _then;
    std::atomic_bool _completed;
    std::mutex _mutex, _thenMutex;

    T _result;
public:
    PromiseTask(std::function<T()> target)
        : _target{target}, _then{nullptr}, _completed{false}
    {}

    PromiseTask(PromiseTask<T>&& other) {
        swap(other);
    }

    PromiseTask<T>& operator=(PromiseTask<T>&& other) {
        swap(other);
        return *this;
    }

    PromiseTask(const PromiseTask<T>&) = delete;
    PromiseTask<T>& operator=(const PromiseTask<T>& other) = delete;

    ~PromiseTask() {
        //std::cerr << ">>Destructed\n";
    }

    bool isReady() {
        return _completed;
    }

    void setThen(std::function<void (T&)> then) {
        std::lock_guard<std::mutex> lock(_thenMutex);
        _then = then;
        if(_completed)
            _then(_result);
    }

    void start() {
        _mutex.lock();
        _result = _target();
        _completed = true;
        _mutex.unlock();

        _thenMutex.lock();
        if(_then)
            _then(_result);
        _thenMutex.unlock();
    }

    std::experimental::optional<T> tryGet() {
        if(_completed)
            return {_result};
        return std::experimental::nullopt;
    }

    T get() {
        // TODO: Remaster
        while(!_completed)
            std::this_thread::yield();
        return _result;
    }

    void swap(PromiseTask<T> &other) {
        std::lock(_mutex, other._mutex, _thenMutex, other._thenMutex);
        std::lock_guard<std::mutex> lock1(_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(other._mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock3(_thenMutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock4(other._thenMutex, std::adopt_lock);

        _completed.exchange(other._isReady);
        std::swap(_target, other._target);
        std::swap(_then, other._then);
        std::swap(_result, other._result);
    }

};

template<class T>
class Promise {
    std::shared_ptr<PromiseTask<T>> _task;
public:
    Promise()
    {}

    Promise(std::function<T()> target)
    {
        auto pool = getMainThreadPool();
        if(pool == nullptr) {
            std::clog << "Executing promise sequentally\n";
            _task = std::shared_ptr<PromiseTask<T>>(new PromiseTask<T>(target));
            _task->start();
        }
        else {
            std::clog << "Executing promise in parallel\n";
            _task = std::shared_ptr<PromiseTask<T>>(new PromiseTask<T>(target));
            pool->addTask([t=_task](){
                t->start();
            });
        }
    }

    Promise(Promise<T>&& other) {
        swap(other);
    }

    Promise<T>& operator=(Promise<T>&& other) {
        swap(other);
        return *this;
    }

    Promise(const Promise<T>&) = delete;
    Promise<T>& operator=(const Promise<T>& other) = delete;

    void then(std::function<void(T&)> thenCb) {
        _task->setThen(thenCb);
    }

    bool isReady() {
        return _task->isReady();
    }

    T result() {
        return _task->get();
    }

    std::experimental::optional<T> tryGet() {
        return _task->tryGet();
    }

private:
    void swap(Promise<T>& other) {
        std::swap(_task, other._task);
    }
};

#endif // PROMISE_H
