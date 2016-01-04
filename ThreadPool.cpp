#include "ThreadPool.hpp"

namespace dbr
{
	namespace cc
	{
		ThreadPool::ThreadPool(std::size_t threadCount)
		:	threadsWaiting(0),
			terminate(false),
			paused(false)
		{
			// keep from reallocating
			threads.reserve(threadCount);

			for(auto i = 0u; i < threadCount; ++i)
				threads.emplace_back(threadTask, this);
		}

		ThreadPool::~ThreadPool()
		{
			stop();
		}

		std::size_t ThreadPool::threadCount() const
		{
			return threads.size();
		}

		std::size_t ThreadPool::waitingJobs() const
		{
			std::lock_guard<std::mutex> jobLock(jobsMutex);
			return jobs.size();
		}

		void ThreadPool::clear()
		{
			std::lock_guard<std::mutex> lock(jobsMutex);

			while(!jobs.empty())
				jobs.pop();
		}

		void ThreadPool::pause()
		{
			paused = true;
		}

		void ThreadPool::resume()
		{
			paused = false;
			jobsAvailable.notify_all();

			// block until at least 1 thread has started
			while(threadsWaiting == threads.size());
		}

		void ThreadPool::wait()
		{
			// we're done waiting once all threads are waiting
			while(threadsWaiting != threads.size());
		}

		void ThreadPool::stop()
		{
			clear();

			// tell threads to stop when they can
			terminate = true;
			jobsAvailable.notify_all();

			// wait for all threads to finish
			for(auto& t : threads)
				t.join();
		}

		void ThreadPool::threadTask(ThreadPool* pool)
		{
			// loop until we break (to keep thread alive)
			while(true)
			{
				if(pool->terminate)
					break;

				std::unique_lock<std::mutex> jobsLock(pool->jobsMutex);

				// if there are no more jobs, or we're paused, go into waiting mode
				if(pool->jobs.empty() || pool->paused)
				{
					++pool->threadsWaiting;
					pool->jobsAvailable.wait(jobsLock, [&]()
					{
						return pool->terminate || !(pool->jobs.empty() || pool->paused);
					});
					--pool->threadsWaiting;

					// if we were woken in order to stop
					if(pool->terminate)
						break;
				}

				// take next job
				auto job = std::move(pool->jobs.front());
				pool->jobs.pop();

				jobsLock.unlock();

				job();
			}
		}
	}
}