#ifndef APPLICATION_H
#define APPLICATION_H

#include "threadpool.h"

class Application {
    std::shared_ptr<ThreadPool> _pool;
    static std::shared_ptr<Application> _current;
    int _status;

    Application();
public:
    static std::shared_ptr<Application> create();
    static std::shared_ptr<Application> getInstance();
    void exit(int status);
    int exec();
    void addTask(std::function<void()> fun);
};

#endif // APPLICATION_H
