#ifndef APPLICATION_H
#define APPLICATION_H

#include "threadpool.h"

#define App Application::getInstance()
#define async __app_async_proxy() += [&]()
#define async_event __app_async_event_proxy() *

class Application {

    static std::shared_ptr<Application> _current;

    std::shared_ptr<ThreadPool> _pool;
    std::atomic_int _status;

    Application();

public:
    static std::shared_ptr<Application> create();
    static std::shared_ptr<Application> getInstance();
    void exit(int status);
    int exec();
    TaskWatcher addTask(std::function<void()> fun);
    TaskWatcher addTask(Task *task);
    TaskWatcher addTask(const std::shared_ptr<Task> &task);

    template<class Callable, class... Args>
    auto add(Callable&& callable, Args&&... args) {
        auto f = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
        return addTask(new Task(f));
    }

    int getThreadId();

    void rescheduleTask();
    void rescheduleTask(const TaskPolicy& policy);
};

class __app_async_proxy {
public:
    auto operator+=(std::function<void()> __f) {
        return App->getInstance()->addTask(__f);
    }
};

template <typename F>
struct lambda_traits
    : public lambda_traits<decltype(&F::operator())>
{};

template<typename Callable, typename Ret, typename... Args>
struct lambda_traits<Ret(Callable::*)(Args...) const> {
public:
    static auto prepare(Callable __f) {
        return std::function<Ret(Args...)>([__f](Args... args) {
            return App->add(__f, std::forward<Args>(args)...);
        });
    }
};

class __app_async_event_proxy {
public:
    /*template<class Ret, class... Args>
    std::function<Ret(Args...)> operator*(std::function<Ret(Args...)> __f) {
        //decltype (__f) proxy = [f = std::forward<Callable>(__f)](auto... args) {
        //    App->add(f, args...);
        //};

        return __f;
    }*/

    template<class Callable>
    auto operator*(Callable __f) {
        return lambda_traits<Callable>::prepare(__f);
    }
};

#endif // APPLICATION_H
