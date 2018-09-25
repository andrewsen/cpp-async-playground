#include "task.h"
#include <iostream>

Task::Task()
    : _boundLooperId{-1}, _bindingPolicy{TaskBindingPolicy::UNBOUND}, _executor{nullptr}, _state{TaskState::PENDING} {
    std::cerr << "Task()\n";
}

Task::Task(Task::Executor executor)
    : _boundLooperId{-1}, _bindingPolicy{TaskBindingPolicy::UNBOUND}, _executor{executor} {
    std::cerr << "Task(Task::Executor executor)\n";
}

Task::Task(Executor executor, int looperId, TaskBindingPolicy policy)
    : _boundLooperId{looperId}, _bindingPolicy{policy}, _executor{executor}, _state{TaskState::PENDING} {
    std::cerr << "Task(Executor executor, int looperId, TaskBindingPolicy policy)\n";
}

Task::~Task() {
    std::cerr << "~Task()\n";
}

TaskBindingPolicy Task::bindingPolicy() const {
    return _bindingPolicy;
}

TaskState Task::getState() const {
    return _state;
}

int Task::boundLooperId() const {
    return _boundLooperId;
}
