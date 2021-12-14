// adapted from https://codereview.stackexchange.com/a/229569
// StackOverflow user KeyC0de 2019
// updates by SO user673679 2019
#pragma once

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>

/// simple worker thread pool
class ThreadPool final {

public:

	/// constructor with max number of threads
	explicit ThreadPool(std::size_t nthreads=std::thread::hardware_concurrency())
		: enabled_(true), pool_(nthreads) {run();}

	~ThreadPool() {wait();}

	// non-copyable
	ThreadPool(ThreadPool const &) = delete;
	ThreadPool& operator=(const ThreadPool &) = delete;

	/// schedule new task to be done in a worker thread
	/// ex. pool.schedule(somefunc);
	/// ex. pool.schedule(std::bind(somefunc, arg1, arg2, ...));
	template<class TaskT>
	auto schedule(TaskT task) -> std::future<decltype(task())> {
		using ReturnT = decltype(task());
		auto promise = std::make_shared<std::promise<ReturnT>>();
		auto result = promise->get_future();
		auto t = [p = std::move(promise), t = std::move(task)]() mutable {execute(*p, t);};
		{
			std::lock_guard<std::mutex> lock(mutex_);
			tasks_.push(std::move(t));
		}
		condvar_.notify_one();
		return result;
	}

private:

	std::mutex mutex_;
	std::condition_variable condvar_;

	bool enabled_;
	std::vector<std::thread> pool_;
	std::queue<std::function<void()>> tasks_;

	template<class ResultT, class TaskT>
	static void execute(std::promise<ResultT> &p, TaskT &task) {
		p.set_value(task());
	}

	template<class TaskT>
	static void execute(std::promise<void> &p, TaskT &task) {
		task();
		p.set_value();
	}

	void wait() {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			enabled_ = false;
		}
		condvar_.notify_all();
		for(auto &t : pool_) {
			t.join();
		}
	}

	void run() {
		auto f = [this]() {
			while(true) {
				std::unique_lock<std::mutex> lock{mutex_};
				condvar_.wait(lock, [&]() {
					return !enabled_ || !tasks_.empty();
				});
				if(!enabled_) {
					break;
				}
				assert(!tasks_.empty());
				auto task = std::move(tasks_.front());
				tasks_.pop();
				lock.unlock();
				task();
			}
		};
		for(auto &t : pool_) {
			t = std::thread(f);
		}
	}
};
