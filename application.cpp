#include <sys/sysinfo.h>

#include "application.h"
#include "threadpool.h"

std::shared_ptr<Application> Application::_current = {nullptr};

Application::Application() : _status{0} {
    if (_current) {
        throw std::runtime_error("Application already created");
    }

    // Get number of cores in the system
    size_t looperCount = static_cast<size_t>(get_nprocs());

    // Create new thread pool that will create looperCount-1 looper threads and use current thread for looper also
    _pool = std::shared_ptr<ThreadPool>(new ThreadPool(looperCount - 1, true));

    setMainThreadPool(_pool);
}

std::shared_ptr<Application> Application::create() {
    if (_current == nullptr)
        _current = std::shared_ptr<Application>(new Application());
    return _current;
}

std::shared_ptr<Application> Application::getInstance() {
    return _current;
}

int Application::exec() {
    // Start thread pool
    _pool->start();
    return _status;
}

TaskWatcher Application::addTask(std::function<void()> fun) {
    return TaskWatcher(_pool->addTask(new Task(fun)));
}

TaskWatcher Application::addTask(Task *task) {
    return TaskWatcher(_pool->addTask(task));
}

TaskWatcher Application::addTask(const std::shared_ptr<Task> &task) {
    return TaskWatcher(_pool->addTask(task));
}

int Application::getThreadId() {
    return getMainThreadPool()->getThisLooper()->getIndex();
}

void Application::rescheduleTask() {
    _pool->getThisLooper()->rescheduleCurrentTask();
}

void Application::rescheduleTask(const TaskPolicy &policy) {
    _pool->getThisLooper()->rescheduleCurrentTask(policy);
}

void Application::exit(int status) {    
    _status = status;

    // Exit task is bound to 0 looper because thread pool can be correctly stopped only from first looper
    // Maybe, need to fix it
    _pool->addTask(new Task([this]() {
                                _pool->stop();
                   }, TaskPolicy {TaskBindingPolicy::BOUND, 0}
    ));
}
