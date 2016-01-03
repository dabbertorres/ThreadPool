#ifndef CC_THREAD_POOL_HPP
#define CC_THREAD_POOL_HPP

#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <vector>
#include <functional>
#include <condition_variable>
#include <queue>

namespace cc
{
	template<typename>
	class ThreadPool;

	template<typename Ret, typename... Args>
	class ThreadPool<Ret(Args...)>
	{
		private:
			using Function = std::function<Ret(Args...)>;
			using PackedTask = std::packaged_task<Ret(Args...)>;
			using Arguments = std::tuple<Args...>;

		public:
			ThreadPool(std::size_t threadCount = 4);

			// calls stop()
			~ThreadPool();

			// non-copyable
			ThreadPool(const ThreadPool&) = delete;
			ThreadPool& operator=(const ThreadPool&) = delete;

			// add a function to be executed, along with any arguments for it
			std::future<Ret> add(const Function& func, Args... args);

			// returns number of threads being used
			std::size_t threadCount() const;

			// blocks calling thread until job queue is empty, leaving ThreadPool usable
			void wait();

			// blocks calling thread until all threads are finished executing their current job
			// leaves ThreadPool unusable
			void stop();

		private:
			// function each thread performs
			static void task(ThreadPool* pool);

			// performs calling of the function for the thread (allowing use of indices from Arugments tuple)
			template<std::size_t... N>
			static void call(PackedTask&& task, Arguments&& args, std::index_sequence<N...>);
			
			std::queue<std::pair<PackedTask, Arguments>> jobs;
			std::mutex jobsMutex;

			// notification variable for waiting threads
			std::condition_variable jobsAvailable;

			std::vector<std::thread> threads;
			std::atomic<std::size_t> threadsWaiting;
			
			std::atomic<bool> terminate;
	};

	template<typename Ret, typename... Args>
	ThreadPool<Ret(Args...)>::ThreadPool(std::size_t threadCount)
	:	threadsWaiting(0),
		terminate(false)
	{
		// keep from reallocating
		threads.reserve(threadCount);

		for(auto i = 0u; i < threadCount; ++i)
			threads.emplace_back(task, this);
	}

	template<typename Ret, typename... Args>
	ThreadPool<Ret(Args...)>::~ThreadPool()
	{
		stop();
	}

	template<typename Ret, typename... Args>
	std::future<Ret> ThreadPool<Ret(Args...)>::add(const Function& func, Args... args)
	{
		PackedTask pt(func);

		// get the future to return
		auto ret = pt.get_future();

		std::lock_guard<std::mutex> lock(jobsMutex);
		jobs.emplace(std::move(pt), std::make_tuple(args...));

		// let a waiting thread know there is an available job
		jobsAvailable.notify_one();
		
		return ret;
	}

	template<typename Ret, typename ...Args>
	std::size_t ThreadPool<Ret(Args...)>::threadCount() const
	{
		return threads.size();
	}

	template<typename Ret, typename ...Args>
	void ThreadPool<Ret(Args...)>::wait()
	{
		// we're done waiting once all threads are waiting
		while(threadsWaiting != threads.size())
		{}
	}

	template<typename Ret, typename ...Args>
	void ThreadPool<Ret(Args...)>::stop()
	{
		// empty job queue
		{
			std::lock_guard<std::mutex> lock(jobsMutex);
			while(!jobs.empty())
				jobs.pop();
		}

		// tell threads to stop when they can
		terminate = true;
		
		// wake up waiting threads to finish
		jobsAvailable.notify_all();

		// wait for all threads to finish
		for(auto& t : threads)
		{
			t.join();
		}
	}

	template<typename Ret, typename... Args>
	void ThreadPool<Ret(Args...)>::task(ThreadPool* pool)
	{
		// loop until we break (to keep thread alive)
		while(true)
		{
			if(pool->terminate)
				break;

			std::unique_lock<std::mutex> queueLock(pool->jobsMutex, std::defer_lock);

			queueLock.lock();

			// if there are no more jobs, go into waiting mode
			if(pool->jobs.empty())
			{
				++pool->threadsWaiting;
				pool->jobsAvailable.wait(queueLock, [&]()
				{
					return !pool->jobs.empty() || pool->terminate;
				});
				--pool->threadsWaiting;

				// if we were woken in order to stop
				if(pool->terminate)
					break;
			}

			// grab next job
			auto pair = std::move(pool->jobs.front());
			PackedTask func = std::move(pair.first);
			Arguments args = std::move(pair.second);
			pool->jobs.pop();

			queueLock.unlock();

			call(std::move(func), std::move(args), std::index_sequence_for<Args...>{});
		}
	}

	template<typename Ret, typename... Args>
	template<std::size_t ...N>
	void ThreadPool<Ret(Args...)>::call(PackedTask&& task, Arguments&& args, std::index_sequence<N...>)
	{
		task(std::get<N>(args)...);
	}
}

#endif
