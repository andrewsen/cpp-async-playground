#ifndef LOOPER_H
#define LOOPER_H

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

#include "task.h"
#include "threadpoolbase.h"
#include "taskqueue.h"

class Looper {
    mutable std::mutex _mutex;

    // Is current loopers stopped
    std::atomic_bool _isStopped;

    // Looper index, id
    const int _index;

    // Global task queue, can be shared between several loopers
    TaskQueue* _globalQueue;

    // Local task queue, accessible and managed only from looper instance
    TaskQueue _localQueue;

    // Synchronizes access to task queues and provides waits
    QueueWatcher& _watcher;

    // ThreadPool instance, used only to reschedule tasks
    ThreadPoolBase* _pool;

    // Should reschedule *current* task after it finishes?
    bool _reschedule;

    // Reschedule policy of *current* task
    std::optional<TaskPolicy> _reschedulePolicy;

public:
    Looper(int index, TaskQueue *queue, QueueWatcher& watcher, ThreadPoolBase* pool);

    ~Looper();

    // Add task to local queue. Tasks from local queue are executed before any other tasks
    void pushBack(const std::shared_ptr<Task> &_currentTask);

    // Get looper index
    int getIndex() const noexcept;

    // Get local queue size
    size_t getQueueSize() const noexcept;

    // Ask looper to finish all local tasks and stop
    void stop() noexcept;

    // Start looper, blocking call
    void loop();

    // Ask looper to execute current task again. Task will be passed to the thread pool,
    // so it might not be executed in current looper
    void rescheduleCurrentTask();
    void rescheduleCurrentTask(const TaskPolicy& policy);

private:
    bool isEmpty();

    // Passes current task to thread pool
    void doReschedule(const std::shared_ptr<Task> &_currentTask);
};

#endif // LOOPER_H
