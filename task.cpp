#include "task.h"
#include <iostream>

std::atomic_size_t Task::_idCounter{1};

Task::Task()
    : _id{_idCounter++}, _policy{}, _executor{nullptr}, _state{TaskState::PENDING} {
    //std::cerr << "Task()\n";
}

Task::Task(Task::Executor executor)
    : _id{_idCounter++}, _policy{}, _executor{executor} {
    //std::cerr << "Task(Task::Executor executor)\n";
}

Task::Task(Executor executor, TaskPolicy policy)
    : _id{_idCounter++}, _policy{policy}, _executor{executor}, _state{TaskState::PENDING} {
    //std::cerr << "Task(Executor executor, int looperId, TaskBindingPolicy policy)\n";
}

Task::~Task() {
    //std::cerr << "~Task()\n";
}

TaskPolicy Task::getPolicy() const {
    return _policy;
}

void Task::setPolicy(const TaskPolicy &policy) {
    _policy = policy;
}

TaskState Task::getState() const {
    return _state;
}

void Task::setState(TaskState state) {
    _state = state;
}

size_t Task::getId() const {
    return _id;
}
