#include "thread_pool.h"

void multi_thread::TaskQueue::push(Task task) {
    std::lock_guard<std::mutex> guard{ mut };
    tasks.emplace(std::move(task));

    cv.notify_one();
}

bool multi_thread::TaskQueue::pop(Task& task) {
    std::unique_lock<std::mutex> ulock{ mut };

    while (tasks.empty() && running) {
        cv.wait(ulock);
    }

    if (tasks.empty() && !running) {
        return false;
    }

    task = std::move(tasks.front());
    tasks.pop();
    return true;
}

void multi_thread::TaskQueue::shutdown() {
    std::lock_guard<std::mutex> guard{ mut };
    running = false;
    cv.notify_all();
}

multi_thread::ThreadPool::ThreadPool(std::vector<std::thread>::size_type workers_num) {
    workers.resize(workers_num);

    for (size_t i = 0; i < workers_num; ++i) {
        workers[i] = std::thread{
            [this]() {
                Task task;
                while (queue.pop(task)) {
                    task();
                }
            }
        };
    }
}

multi_thread::ThreadPool::~ThreadPool() {
    shutdown();
}

void multi_thread::ThreadPool::submit(Task task) {
    queue.push(task);
}

void multi_thread::ThreadPool::shutdown() {
    queue.shutdown();

    for (std::thread& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}