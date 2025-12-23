#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>



class ThreadPool 
{
public:

    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
        : num_threads_(num_threads), stop_(true)
    {
        if (num_threads == 0) 
        {
            num_threads_ = 1; // at least one thread
        }
    }

    ~ThreadPool() 
    {
        stop();
    }

    // delete copy constuctor and operator=
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;


    // start thread pool
    void start() 
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (!stop_) 
        {
            return;
        }

        stop_ = false;

        // create threads
        threads_.reserve(num_threads_);
        for (size_t i = 0; i < num_threads_; ++i) 
        {
            threads_.emplace_back([this] 
                {
                    while (true) 
                    {
                        std::function<void()> task;

                        // mutex
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex_);

                            // wait for new task
                            this->cv_.wait(lock, [this]{ return !this->tasks_.empty() || this->stop_; });


                            if (this->stop_) 
                            {
                                return; // finish thread
                            }

 
                            task = std::move(this->tasks_.front());
                            this->tasks_.pop();
                        }

                        // execute task
                        task();
                    }
                });
        }
    }

    // stop thread pool
    void stop() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) 
            {
                return;
            }

            stop_ = true;

            // clear queue of tasks
            std::queue<std::function<void()>> empty_queue;
            std::swap(tasks_, empty_queue);
        }

        // notify threads
        cv_.notify_all();

        for (auto& thread : threads_) 
        {
            if (thread.joinable()) 
            {
                thread.join();
            }
        }

        threads_.clear();
    }


    // add task to queue
    void enqueue(std::function<void()> task) 
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // if stopped, ignore task
            if (stop_) 
            {
                return;
            }

            tasks_.emplace(std::move(task));
        }
        cv_.notify_one();
    }


private:

    size_t num_threads_;
    std::vector<std::thread> threads_;

    std::queue<std::function<void()>> tasks_;

    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_;
};
