#pragma once

#include <functional>
#include <queue>
#include <thread>
#include <condition_variable>
#include <vector>

namespace multi_thread {
    using Task = std::function<void()>;

    class TaskQueue {
        std::queue<Task> tasks;
        std::mutex mut;
        std::condition_variable cv;
        bool running = true;
    public:
        void push(Task task);
        bool pop(Task& task);
        void shutdown();
    };

    class ThreadPool {
        std::vector<std::thread> workers;
        TaskQueue queue;
    public:
        ThreadPool(std::vector<std::thread>::size_type workers_num);

        ~ThreadPool();

        void submit(Task task);

        void shutdown();
    };
}
