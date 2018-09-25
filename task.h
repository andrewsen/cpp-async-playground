#ifndef TASK_H
#define TASK_H

#include <atomic>
#include <functional>

enum class TaskBindingPolicy { UNBOUND, UNBOUND_EXCEPT, BOUND };

enum class TaskState { PENDING, EXECUTING, FINISHED };

class Task {
protected:
    typedef std::function<void()> Executor;

private:
    const int _boundLooperId;
    const TaskBindingPolicy _bindingPolicy;
    const Executor _executor;

    std::atomic<TaskState> _state;

public:
    Task();

    Task(Executor executor);

    Task(Executor executor, int looperId, TaskBindingPolicy policy = TaskBindingPolicy::BOUND);

    virtual ~Task();

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    int boundLooperId() const;

    TaskBindingPolicy bindingPolicy() const;

    TaskState getState() const;

    void execute() {
        _state = TaskState::EXECUTING;
        _executor();
        _state = TaskState::FINISHED;
    }

    void operator()() {
        execute();
    }

    // template <class Ret, class... Args>
    // void setFunction(std::function<Ret(Args...)> func);
};

#endif // TASK_H
