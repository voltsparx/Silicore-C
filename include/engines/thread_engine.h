#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace silicore::engines {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto submit(F&& fn, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using Result = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<Result()>>(
            std::bind(std::forward<F>(fn), std::forward<Args>(args)...)
        );
        std::future<Result> future = task->get_future();
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return future;
    }

    void shutdown();

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

} // namespace silicore::engines
