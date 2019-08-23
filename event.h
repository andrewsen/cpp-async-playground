#ifndef EVENT_H
#define EVENT_H

#include <mutex>
#include <list>

#include "application.h"

template <class F, class... Args>
class Event {
    using Callback = std::function<void(Args...)>;
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

    void operator+=(Callback callback) {
        std::lock_guard<std::mutex> locker(_mutex);
        _handlers.push_back(callback);
    }

    void operator-=(Callback callback) {
        std::lock_guard<std::mutex> locker(_mutex);
        _handlers.remove(callback);
    }

protected:
    virtual void invoke(Args... args) {
        std::lock_guard<std::mutex> locker(_mutex);
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
        auto pool = getMainThreadPool();

        std::lock_guard<std::mutex> locker(this->_mutex);
        for (auto handler : this->_handlers) {
            Application::getInstance()->add(handler, args...);
        }
    }

    void invokeSync(Args... args) {
        for (auto handler : this->_handlers) {
            handler(args...);
        }
    }
};

#endif // EVENT_H
