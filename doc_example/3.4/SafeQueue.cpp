#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class SafeQueue {
private:
    std::queue<T> q;
    mutable std::mutex mtx;
    std::condition_variable cond;
public:
    SafeQueue() {}

    void enqueue(T t) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(t);
        cond.notify_one();
    }

    T dequeue() {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [this](){ return !q.empty(); });
        T val = q.front();
        q.pop();
        return val;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return q.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return q.size();
    }
};
