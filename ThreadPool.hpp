#ifndef DBR_CC_THREAD_POOL_HPP
#define DBR_CC_THREAD_POOL_HPP

#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <vector>
#include <functional>
#include <condition_variable>
#include <queue>

namespace dbr
{
	namespace cc
	{
		class ThreadPool
		{
			public:
				ThreadPool(std::size_t threadCount = 4);

				// non-copyable
				ThreadPool(const ThreadPool&) = delete;
				ThreadPool& operator=(const ThreadPool&) = delete;

				// movable
				ThreadPool(ThreadPool&&) = default;
				ThreadPool& operator=(ThreadPool&&) = default;

				// calls stop() below
				~ThreadPool();
				
				// add a function to be executed, along with any arguments for it
				template<typename Func, typename... Args>
				auto add(Func&& func, Args... args) -> std::future<typename std::result_of<Func(Args...)>::type>;

				// returns number of threads being used
				std::size_t threadCount() const;

				// returns the number of jobs waiting to be executed
				std::size_t waitingJobs() const;

				// clears currently queued jobs (jobs which are not currently running), ThreadPool will still be usable
				void clear();

				// pause and resume job execution. Does not affect currently running jobs
				void pause();
				void resume();

				// blocks calling thread until job queue is empty, leaving ThreadPool usable
				void wait();

				// blocks calling thread until all threads are finished executing their current job
				// leaves ThreadPool unusable
				void stop();

			private:
				using Job = std::function<void()>;

				// function each thread performs
				static void threadTask(ThreadPool* pool);

				// using a templated struct in order to get partial specialization capability for the return type

				// default specialization
				template<typename Ret, typename Func, typename... Args>
				struct Call
				{
					void operator()(const Func& func, const std::tuple<Args...>& args, const std::shared_ptr<std::promise<Ret>>& promise);
					
					template<std::size_t... Is>
					static void helper(Ret& ret, const Func& func, const std::tuple<Args...>& args, std::index_sequence<Is...>);
				};

				// void specialization
				template<typename Func, typename... Args>
				struct Call<void, Func, Args...>
				{
					void operator()(const Func& func, const std::tuple<Args...>& args, const std::shared_ptr<std::promise<void>>& promise);

					template<std::size_t... Is>
					static void helper(const Func& func, const std::tuple<Args...>& args, std::index_sequence<Is...>);
				};

				std::queue<Job> jobs;
				mutable std::mutex jobsMutex;

				// notification variable for waiting threads
				std::condition_variable jobsAvailable;

				std::vector<std::thread> threads;
				std::atomic<std::size_t> threadsWaiting;

				std::atomic<bool> terminate;
				std::atomic<bool> paused;
		};

		template<typename Func, typename... Args>
		auto ThreadPool::add(Func&& func, Args... args) -> std::future<typename std::result_of<Func(Args...)>::type>
		{
			using Ret = typename std::result_of<Func(Args...)>::type;

			auto promise = std::make_shared<std::promise<Ret>>();

			// get the future to return later
			std::future<Ret> ret = promise->get_future();

			Job wrapped = std::bind(Call<Ret, Func, Args...>{}, func, std::make_tuple(args...), promise);

			std::lock_guard<std::mutex> lock(jobsMutex);
			jobs.emplace(wrapped);

			// let a waiting thread know there is an available job
			jobsAvailable.notify_one();

			return ret;
		}

		// default specialization
		template<typename Ret, typename Func, typename... Args>
		void ThreadPool::Call<Ret, Func, Args...>::operator()(const Func& func, const std::tuple<Args...>& args, const std::shared_ptr<std::promise<Ret>>& promise)
		{
			Ret ret;
			helper(ret, func, args, std::index_sequence_for<Args...>{});
			promise->set_value(ret);
		}

		template<typename Ret, typename Func, typename... Args>
		template<std::size_t... Is>
		void ThreadPool::Call<Ret, Func, Args...>::helper(Ret& ret, const Func& func, const std::tuple<Args...>& args, std::index_sequence<Is...>)
		{
			ret = func(std::get<Is>(args)...);
		}

		// void specialization
		template<typename Func, typename... Args>
		void ThreadPool::Call<void, Func, Args...>::operator()(const Func& func, const std::tuple<Args...>& args, const std::shared_ptr<std::promise<void>>& promise)
		{
			helper(func, args, std::index_sequence_for<Args...>{});
			promise->set_value();
		}

		template<typename Func, typename... Args>
		template<std::size_t... Is>
		void ThreadPool::Call<void, Func, Args...>::helper(const Func& func, const std::tuple<Args...>& args, std::index_sequence<Is...>)
		{
			func(std::get<Is>(args)...);
		}
	}
}

#endif
