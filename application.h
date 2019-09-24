#ifndef APPLICATION_H
#define APPLICATION_H

#include "threadpool.h"

// Some defines to ease usage
#define App Application::getInstance()
#define async __app_async_proxy() += [&]()
#define async_event __app_async_event_proxy() *

// Singletone class for high-level async operations
class Application {
    static std::shared_ptr<Application> _current;

    std::shared_ptr<ThreadPool> _pool;

    // Return code is stored here
    std::atomic_int _status;

    Application();

public:
    static std::shared_ptr<Application> create();

    static std::shared_ptr<Application> getInstance();

    // Creates termination task to finish the app
    void exit(int status);

    // Starts application loopers, blocking call
    int exec();

    TaskWatcher addTask(std::function<void()> fun);

    TaskWatcher addTask(Task *task);

    TaskWatcher addTask(const std::shared_ptr<Task> &task);

    // Add arbitary callable to execution queue
    template<class Callable, class... Args>
    auto add(Callable&& callable, Args&&... args) {
        auto f = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
        return addTask(new Task(f));
    }

    int getThreadId();

    // Reschedules *current* task with same policies (literally pushes to task queue)
    void rescheduleTask();

    // Reschedules *current* task with new policies
    void rescheduleTask(const TaskPolicy& policy);
};

// Wrapper class to enable += operator for adding new tasks
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

// Used to separate return and argument types of function-lile entities
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
    template<class Callable>
    auto operator*(Callable __f) {
        return lambda_traits<Callable>::prepare(__f);
    }
};

#endif // APPLICATION_H
