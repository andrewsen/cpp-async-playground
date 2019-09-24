#ifndef TASK_H
#define TASK_H

#include <atomic>
#include <functional>
#include <memory>

enum class TaskBindingPolicy {
    // Can be executed in any thread (looper)
    UNBOUND,

    // Can be executed in any thread (looper) except specified one
    UNBOUND_EXCEPT,

    // Should be executed only in a specific thread
    BOUND
};

enum class TaskState { PENDING, EXECUTING, FINISHED, CANCELED };

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

    // Provides unique task id
    static std::atomic_size_t _idCounter;
public:
    Task() noexcept;

    Task(Executor executor) noexcept;

    Task(Executor executor, TaskPolicy policy) noexcept;

    virtual ~Task() = default;

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    TaskPolicy getPolicy() const noexcept;
    void setPolicy(const TaskPolicy &policy) noexcept;

    TaskState getState() const noexcept;
    void setState(TaskState state) noexcept;

    size_t getId() const noexcept;

    void execute();

    void operator()();
};


// Safe wrapper above the task
class TaskWatcher {
private:
    const std::shared_ptr<Task> _task;

public:
    TaskWatcher(const std::shared_ptr<Task> task) noexcept;

    auto getState() const noexcept;

    auto getId() const noexcept;

    auto getPolicy() const noexcept;

    bool isFinished() const noexcept;

    void cancel() noexcept;
};

#endif // TASK_H
