#ifndef EVENT_H
#define EVENT_H

#include <mutex>
#include <list>

#include "application.h"

// Cosplay of C# event mechanism
template <class F, class... Args>
class Event {
    using Callback = std::function<void(Args...)>;

    // It's not good, but for now I can't find better way to give only to event-hosting class an access to some members
    friend F;

protected:
    mutable std::mutex _mutex;
    std::list<Callback> _handlers;

public:
    Event() {}

    Event(const Event &other) {
        std::lock_guard<std::mutex> lock(other._mutex);
        _handlers = other._handlers;
    }


    virtual ~Event() {}

    Event &operator=(const Event &other) {
        std::lock_guard<std::mutex> lock(_mutex);
        _handlers.clear();

        other._mutex.lock();
        _handlers = other._handlers;
        other._mutex.unlock();
        return *this;
    }

    // C#-like addition operator (subscribes a callback to the event)
    void operator+=(Callback callback) {
        std::lock_guard<std::mutex> locker(_mutex);
        _handlers.push_back(callback);
    }

    // C#-like remove operator (unsubscribes a callback from the event)
    void operator-=(Callback callback) {
        std::lock_guard<std::mutex> locker(_mutex);
        _handlers.remove(callback);
    }

protected:
    virtual void invoke(Args... args) {
        std::lock_guard<std::mutex> locker(_mutex);

        // Simply iterate all subscribed callbacks
        for (auto handler : _handlers)
            handler(args...);
    }

    void operator()(Args... args) {
        invoke(args...);
    }

    void swap(Event &other) {
        std::lock(_mutex, other._mutex);
        std::lock_guard<std::mutex> lock_a(_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock_b(other._mutex, std::adopt_lock);

        std::swap(_handlers, other._handlers);
    }
};

template <class F, class... Args>
class AsyncEvent : public Event<F, Args...> {
    friend F;

protected:
    virtual void invoke(Args... args) override {
        // Instead of invoking callbacks directly, add them to the application as tasks
        std::lock_guard<std::mutex> locker(this->_mutex);
        for (auto handler : this->_handlers) {
            Application::getInstance()->add(handler, args...);
        }
    }

    void invokeSync(Args... args) {
        // Just pass to base sync invoke
        Event<F, Args...>::invoke(args...);
    }
};

#endif // EVENT_H
