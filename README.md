# ThreadPool
A thread pool for C++ using std::thread and other C++11/14 standard threading utilities.

Easy to use, with some extra functions for a bit more control.

* #### `auto add(Func&& func, Args&&... args)`
	This is how jobs are added to the `ThreadPool`. Accepts a callable object (`func`) to be queued, along with arguments (`args...`). Returns a `std::future` templated to the return type of `Func`, to obtain the return value.
	
	Full signature:
	```cpp
	template<typename Func, typename... Args>
	auto add(Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type>;
	```

* #### `std::size_t threadCount() const`
	Returns what the name implies: the number of threads used.

* #### `std::size_t waitingJobs() const`
	Returns the number of jobs currently queued (not currently executing).

* #### `Ids ids() const`
	Returns a list of the `std::thread::id`s assigned to the threads being used by the `ThreadPool`.

	```cpp
	using Ids = std::vector<std::thread::id>;
	```

* #### `void clear()`
	Clears the queue of waiting jobs. Does not affect currently executing jobs.

* #### `void pause(bool state)`
	Upon being called:
	if `state` is true, prevents threads from grabbing another job upon completion of their current job until `pause(false)` is called.
	
	if `state` is false, allows threads to grab more jobs as they finish. This is the default state.

	Note: calling `wait()` will not *wait* if `pause(true)` was called last (ie: call `pause(false)` after `pause(true)` and before `wait()`).

* #### `void wait()`
	Blocks until the number of threads waiting for a job is equal to the number of threads (ie: until all threads have nothing to execute).

* #### Destructor Note
	The destructor does not wait for queued jobs to be finished. It first clears the queue (call to `clear()`), tells threads to close, wakes up any waiting threads, and then joins to all of them. If you want to wait until the `ThreadPool` is done, call `wait()` before its lifetime ends.
