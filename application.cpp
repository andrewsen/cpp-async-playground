#include <sys/sysinfo.h>

#include "application.h"
#include "threadpool.h"

std::shared_ptr<Application> Application::_current = {nullptr};

Application::Application() : _status{0} {
    if (_current) {
        throw std::runtime_error("Application already created");
    }

    size_t looperCount = get_nprocs();

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
    _pool->addTask(new Task([this]() {
                                _pool->stop();
                   }, TaskPolicy {TaskBindingPolicy::BOUND, 0}
    ));
    //_pool->stop();
}
