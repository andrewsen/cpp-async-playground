#ifndef PROMISE_H
#define PROMISE_H

#include "application.h"
#include <condition_variable>
#include <optional>
#include <functional>
#include <tuple>

template<class T>
class PromiseTask : Task {
    using Thennable = std::function<void(T)>;

    Thennable _then;
    TaskBindingPolicy _thenBindingPolicy;
    const int _thenBoundLooperId;
    std::mutex _thenMutex;
    //T _result;
    uint8_t _resultBlob[sizeof(T)];

public:
    template<class Callable, class... Args>
    PromiseTask(Callable&& callback, Args&&... args)
        : Task{createExecutor(std::forward<Callable>(callback), std::forward<Args>(args)...)},
          _thenBoundLooperId {-1}, _thenBindingPolicy {TaskBindingPolicy::UNBOUND}, _then {nullptr} {
        std::cerr << "PromiseTask(Callable&& callback, Args&&... args)\n";
    }

    template<class Callable, class... Args>
    PromiseTask(Callable&& callback, int looperId, TaskBindingPolicy policy, int thenLooperId,
                   TaskBindingPolicy thenPolicy, Args&&... args)
        : Task{createExecutor(std::forward<Callable>(callback), std::forward<Args>(args)...), looperId, policy},
          _thenBoundLooperId {thenLooperId}, _thenBindingPolicy {thenPolicy}, _then {nullptr} {
        std::cerr << "PromiseTask(Callable&& callback, int looperId, TaskBindingPolicy policy, int thenLooperId, TaskBindingPolicy thenPolicy, Args&&... args)\n";
    }

    void setThen(Thennable then) {
        std::lock_guard<std::mutex> lock(_thenMutex);
        _then = then;
        if (isReady()) {
            Application::getInstance()->addTask(new Task([result = std::move(*(T*)(_resultBlob)), then = _then]() { then(result); },
                                                         _thenBoundLooperId, _thenBindingPolicy));
        }
    }

    virtual ~PromiseTask() {
        std::cerr << "~PromiseTask()\n";
    }

    T get() const {
        while(this->getState() != TaskState::FINISHED) {
            std::this_thread::yield();
        }
        return *(T*)(_resultBlob);
    }

    std::optional<T> tryGet() const {
        if (this->getState() == TaskState::FINISHED)
            return {*(T*)(_resultBlob)};
        return std::nullopt;
    }

    bool isReady() {
        return this->getState() == TaskState::FINISHED;
    }

private:
    template<class Callable, class... Args>
    Executor createExecutor(Callable&& callback, Args&&... args) {
        auto argsTuple = std::make_tuple<Args...>(std::forward<Args>(args)...);
        return [this, callback = std::forward<Callable>(callback), args = std::move(argsTuple)]() mutable {
            execInternal(callback, std::forward<decltype (args)>(args));
        };
    }

    template<class Callable, class... Args>
    void execInternal(Callable&& callback, std::tuple<Args...>&& args) {
        *(T*)(_resultBlob) = std::apply(callback, std::forward<decltype (args)>(args));

        _thenMutex.lock();
        if (_then) {
            Application::getInstance()->addTask(new Task([result = std::move(*(T*)(_resultBlob)), then = _then]() mutable {
                then(std::forward<T>(result));
            }, _thenBoundLooperId, _thenBindingPolicy));
        }
        _thenMutex.unlock();
    }
};

template<class T>
class Promise {
    std::shared_ptr<Task> _task;

public:
    template<class Callable, class... Args>
    Promise(Callable&& target, Args&&... args) {
        auto app = Application::getInstance();
        _task = std::shared_ptr<Task>((Task*)new PromiseTask<T> {
                                          std::forward<Callable>(target),
                                          std::forward<Args>(args)...
                                      });
        app->addTask(_task);
    }

    void then(std::function<void(T)> thenCb) {
        promise_cast()->setThen(thenCb);
    }

    bool isReady() {
        return promise_cast()->isReady();
    }

    T result() {
        return promise_cast()->get();
    }

    std::optional<T> tryGet() {
        return promise_cast()->tryGet();
    }

private:
    void swap(Promise<T>& other) {
        std::swap(_task, other._task);
    }

    PromiseTask<T>* promise_cast() {
        return ((PromiseTask<T>*)_task.get());
    }
};

#endif // PROMISE_H
