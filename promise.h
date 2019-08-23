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

    TaskPolicy _thenPolicy;
    Thennable _then;
    std::mutex _thenMutex;
    //T _result;
    uint8_t _resultBlob[sizeof(T)];

public:
    template<class Callable, class... Args>
    PromiseTask(Callable&& callback, Args&&... args)
        : Task{createExecutor(std::forward<Callable>(callback), std::forward<Args>(args)...)},
          _thenPolicy{}, _then {nullptr} {
        //std::cerr << "PromiseTask(Callable&& callback, Args&&... args)\n";
    }

    template<class Callable, class... Args>
    PromiseTask(Callable&& callback, const TaskPolicy& taskPolicy, const TaskPolicy& thenPolicy, Args&&... args)
        : Task{createExecutor(std::forward<Callable>(callback), std::forward<Args>(args)...), taskPolicy},
          _thenPolicy{thenPolicy}, _then {nullptr} {
        //std::cerr << "PromiseTask(Callable&& callback, int looperId, TaskBindingPolicy policy, int thenLooperId, TaskBindingPolicy thenPolicy, Args&&... args)\n";
    }

    virtual ~PromiseTask() {
        //std::cerr << "~PromiseTask()\n";
    }

    void setThen(Thennable then) {
        std::lock_guard<std::mutex> lock(_thenMutex);
        _then = then;
        if (isReady()) {
            Application::getInstance()->addTask(new Task([result = std::move(*(T*)(_resultBlob)), then = _then]() {
                then(result);
            }, _thenPolicy));
        }
    }

    T get() const {
        return *(T*)(_resultBlob);
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
            }, _thenPolicy));
        }
        _thenMutex.unlock();
    }
};

template<>
class PromiseTask<void> : Task {
    using Thennable = std::function<void()>;

    TaskPolicy _thenPolicy;
    Thennable _then;
    std::mutex _thenMutex;

public:
    template<class Callable, class... Args>
    PromiseTask(Callable&& callback, Args&&... args)
        : Task{createExecutor(std::forward<Callable>(callback), std::forward<Args>(args)...)},
          _thenPolicy{}, _then {nullptr} {
        //std::cerr << "PromiseTask(Callable&& callback, Args&&... args)\n";
    }

    template<class Callable, class... Args>
    PromiseTask(Callable&& callback, const TaskPolicy& taskPolicy, const TaskPolicy& thenPolicy, Args&&... args)
        : Task{createExecutor(std::forward<Callable>(callback), std::forward<Args>(args)...), taskPolicy},
          _thenPolicy{thenPolicy}, _then {nullptr} {
        //std::cerr << "PromiseTask(Callable&& callback, int looperId, TaskBindingPolicy policy, int thenLooperId, TaskBindingPolicy thenPolicy, Args&&... args)\n";
    }

    virtual ~PromiseTask() {
        //std::cerr << "~PromiseTask()\n";
    }

    void setThen(Thennable then) {
        std::lock_guard<std::mutex> lock(_thenMutex);
        _then = then;
        if (isReady()) {
            Application::getInstance()->addTask(new Task(_then, _thenPolicy));
        }
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
        std::apply(callback, std::forward<decltype (args)>(args));

        _thenMutex.lock();
        if (_then) {
            Application::getInstance()->addTask(new Task(_then, _thenPolicy));
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
        while (!promise_cast()->isReady())
            std::this_thread::yield();
        return promise_cast()->get();
    }

    std::optional<T> tryGet() {
        if (promise_cast()->isReady())
            return { promise_cast()->get() };
        return std::nullopt;
    }

private:
    PromiseTask<T>* promise_cast() const {
        return ((PromiseTask<T>*)_task.get());
    }
};

template<>
class Promise<void> {
    std::shared_ptr<Task> _task;

public:
    template<class Callable, class... Args>
    Promise(Callable&& target, Args&&... args) {
        auto app = Application::getInstance();
        _task = std::shared_ptr<Task>((Task*)new PromiseTask<void> {
                                          std::forward<Callable>(target),
                                          std::forward<Args>(args)...
                                      });
        app->addTask(_task);
    }

    void then(std::function<void()> thenCb) {
        promise_cast()->setThen(thenCb);
    }

    bool isReady() {
        return promise_cast()->isReady();
    }

private:
    PromiseTask<void>* promise_cast() {
        return ((PromiseTask<void>*)_task.get());
    }
};

#endif // PROMISE_H
