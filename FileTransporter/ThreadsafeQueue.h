#pragma once

#include <condition_variable>
#include <mutex>
#include <deque>

namespace ft
{

	template <typename T>
	class TSQueue
	{
	public:
		TSQueue() = default;
		// delete copy constructor
		TSQueue(const TSQueue<T>&) = delete;
		virtual ~TSQueue() { clear(); }

		const T& front()
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			return deque.front();
		}
		const T& back()
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			return deque.back();
		}

		void push_back(const T& item)
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			deque.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(mtxWaiting);
			cvWaiting.notify_one();
		}
		void push_front(const T& item)
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			deque.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(mtxWaiting);
			cvWaiting.notify_one();
		}

		bool empty()
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			return deque.empty();
		}

		size_t count()
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			return deque.size();
		}

		void clear()
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			deque.clear();
		}

		T pop_back()
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			auto item = std::move(deque.back());
			deque.pop_back();
			return item;
		}
		T pop_front()
		{
			std::lock_guard<std::mutex> guard(mtxQueue);
			auto item = std::move(deque.front());
			deque.pop_front();
			return item;
		}

		void wait()
		{
			while (empty() && !is_force_stop)
			{
				std::unique_lock<std::mutex> ul(mtxWaiting);
				cvWaiting.wait(ul);
			}
		}

		void wakeUpCV()
		{
			cvWaiting.notify_all();
			is_force_stop = true;
		}

	protected:
		std::mutex mtxQueue;
		std::deque<T> deque;

		std::condition_variable cvWaiting;
		std::mutex mtxWaiting;

		std::atomic<bool> is_force_stop = false;

	};

}
