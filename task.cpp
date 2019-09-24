#include "task.h"
#include <iostream>

std::atomic_size_t Task::_idCounter{1};

Task::Task() noexcept
    : _id{_idCounter++}, _policy{}, _executor{nullptr}, _state{TaskState::PENDING} {
}

Task::Task(Task::Executor executor) noexcept
    : _id{_idCounter++}, _policy{}, _executor{executor} {
}

Task::Task(Executor executor, TaskPolicy policy) noexcept
    : _id{_idCounter++}, _policy{policy}, _executor{executor}, _state{TaskState::PENDING} {
}

TaskPolicy Task::getPolicy() const noexcept {
    return _policy;
}

void Task::setPolicy(const TaskPolicy &policy) noexcept {
    _policy = policy;
}

TaskState Task::getState() const noexcept {
    return _state;
}

void Task::setState(TaskState state) noexcept {
    _state = state;
}

size_t Task::getId() const noexcept {
    return _id;
}

void Task::execute() {
    _state = TaskState::EXECUTING;
    if(_executor)
        _executor();
    _state = TaskState::FINISHED;
}

void Task::operator()() {
    execute();
}

TaskWatcher::TaskWatcher(const std::shared_ptr<Task> task) noexcept
    : _task{task}
{}

auto TaskWatcher::getState() const noexcept {
    return _task->getState();
}

auto TaskWatcher::getId() const noexcept {
    return _task->getId();
}

auto TaskWatcher::getPolicy() const noexcept {
    return _task->getPolicy();
}

bool TaskWatcher::isFinished() const noexcept {
    auto state = _task->getState();
    return state == TaskState::FINISHED || state == TaskState::CANCELED;
}

void TaskWatcher::cancel() noexcept {
    _task->setState(TaskState::CANCELED);
}
