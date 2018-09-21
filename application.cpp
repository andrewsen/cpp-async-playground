#include <sys/sysinfo.h>

#include "application.h"
#include "threadpool.h"

std::shared_ptr<Application> Application::_current = {nullptr};

Application::Application()
    : _status{0} {
    if(_current) {
        throw std::runtime_error("Application already created");
    }

    _pool = std::shared_ptr<ThreadPool>(new ThreadPool(get_nprocs()-1, true));
    setMainThreadPool(_pool);
}

std::shared_ptr<Application> Application::create()
{
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

void Application::addTask(std::function<void ()> fun) {
    _pool->addTask(fun);
}

void Application::exit(int status) {
    _pool->stop();
    _status = status;
}
