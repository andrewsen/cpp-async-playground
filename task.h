#ifndef TASK_H
#define TASK_H

#include <atomic>
#include <functional>
#include <memory>

enum class TaskBindingPolicy { UNBOUND, UNBOUND_EXCEPT, BOUND };

enum class TaskState { PENDING, EXECUTING, FINISHED, CANCELED };

//typedef std::optional<int> LooperPolicyBinding;

struct TaskPolicy {
    TaskBindingPolicy policy;
    int boundLooper;

    TaskPolicy()
        : policy {TaskBindingPolicy::UNBOUND}, boundLooper {-1}
    {}

    TaskPolicy (TaskBindingPolicy policy)
        : policy {policy}, boundLooper {-1}
    {}

    TaskPolicy (TaskBindingPolicy policy, int looper)
        : policy {policy}, boundLooper {looper}
    {}
};

class Task {
protected:
    typedef std::function<void()> Executor;

private:
    const size_t _id;
    TaskPolicy _policy;
    const Executor _executor;
    std::atomic<TaskState> _state;

    static std::atomic_size_t _idCounter;
public:
    Task();

    Task(Executor executor);

    Task(Executor executor, TaskPolicy policy);

    virtual ~Task();

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    TaskPolicy getPolicy() const;
    void setPolicy(const TaskPolicy &policy);

    TaskState getState() const;
    void setState(TaskState state);

    size_t getId() const;

    void execute() {
        _state = TaskState::EXECUTING;
        if(_executor)
            _executor();
        _state = TaskState::FINISHED;
    }

    void operator()() {
        execute();
    }

    // template <class Ret, class... Args>
    // void setFunction(std::function<Ret(Args...)> func);
};

class TaskWatcher {
private:
    const std::shared_ptr<Task> _task;

public:
    TaskWatcher(const std::shared_ptr<Task> task)
        : _task{task}
    {}

    auto getState() const {
        return _task->getState();
    }

    auto getId() const {
        return _task->getId();
    }

    auto getPolicy() const {
        return _task->getPolicy();
    }

    bool isFinished() const {
        auto state = _task->getState();
        return state == TaskState::FINISHED || state == TaskState::CANCELED;
    }

    void cancel() {
        _task->setState(TaskState::CANCELED);
    }
};

#endif // TASK_H
