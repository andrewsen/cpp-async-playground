#include "looper.h"

Looper::Looper(int index)
    : _isStopped{false}, _index{index}, _warmUpCache {nullptr}
{}

Looper::Looper(Looper &&other)
    : _index{other._index} {
    swap(other);
}

Looper::~Looper() {
    std::clog << "~Looper destructed\n";
    _isStopped = true;
    _mutex.lock();
    while(!_queue.empty())
        _queue.pop_back();

}

void Looper::pushBack(Looper::Callback callback) {
    std::lock_guard<std::mutex> guard(_mutex);
    //if (_queue.empty() && _warmUpCache == nullptr)
    //    _warmUpCache = callback;
    //else
        _queue.push_back(callback);
}

size_t Looper::getQueueSize() const noexcept {
    std::lock_guard<std::mutex> guard(_mutex);
    return _queue.size();
}

void Looper::moveTo(int count, std::shared_ptr<Looper> to) {
    if(count == 0)
        return;

    std::lock_guard<std::mutex> guard(_mutex);
    if (count > _queue.size())
        count = _queue.size();
    std::clog << "Moving " << count << " tasks from #" << _index << " (" << _queue.size() << ") to #" << to->_index << " (" << to->_queue.size() << ")\n";
    std::move(std::next(_queue.begin(), _queue.size() - count), _queue.end(), std::back_inserter(to->_queue));
    _queue.resize(_queue.size() - count);
    std::clog << "Now #" << _index << " has " << _queue.size() << " tasks, #" << to->_index << " has " << to->_queue.size() << " tasks\n";
}

int Looper::getIndex() const noexcept {
    return _index;
}

void Looper::stop() noexcept {
    _isStopped = true;
}

void Looper::loop(bool reentrant) {
    Callback currentFunc = nullptr;
    while(!_isStopped || !isSafeEmpty()) {
        _mutex.lock();
        if(!_queue.empty()) {
            currentFunc = _queue.front();
            _queue.pop_back();
            _mutex.unlock();
        }
        //else if(_warmUpCache != nullptr) {
        //    currentFunc = _warmUpCache;
        //    _warmUpCache = nullptr;
        //    _mutex.unlock();
        //}
        else {
            currentFunc = nullptr;
            _mutex.unlock();
            if (reentrant)
                return;
        }
        if(currentFunc) {
            //std::clog << "start currentFunc()\n";
            currentFunc();
            //std::clog << "end currentFunc()\n";
        }
    }
}

bool Looper::isSafeEmpty() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _queue.empty();
}

void Looper::swap(Looper& other) {
    std::lock(_mutex, other._mutex);
    std::lock_guard<std::mutex> lock_a(_mutex, std::adopt_lock);
    std::lock_guard<std::mutex> lock_b(other._mutex, std::adopt_lock);

    std::swap(_queue, other._queue);
    _isStopped.exchange(other._isStopped);
    //std::swap(_isStopped, other._isStopped);
}
