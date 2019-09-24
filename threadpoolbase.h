#ifndef THREADPOOLBASE_H
#define THREADPOOLBASE_H

#include <memory>

#include "task.h"

class ThreadPoolBase {
public:
    virtual std::shared_ptr<Task> addTask(Task *task) = 0;
    virtual std::shared_ptr<Task> addTask(const std::shared_ptr<Task> &task) = 0;

    virtual ~ThreadPoolBase();
};

#endif // THREADPOOLBASE_H
