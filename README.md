# ThreadPool
A thread pool for C++ using std::thread and other C++11/14 standard threading utilities.

Templated class, one, the return type of your function, the second, a variadic template of arguments to the function.

Pretty easy to use. There is all of 4 public functions:
std::future<Ret> add(const Function& func, Args... args)
	- Signature should be somewhat self-explanatory. Accepts a function to be queued, along with all the arguments to it. Returns a std::future, so you can get the return value if so desired.
	
std::size_t threadCount() const
	- Returns what the name implies: the number of threads used. 
	
void wait()
	- Does what the name implies. Blocks until all threads are waiting for a job (until all queued jobs are complete, ie: queue is empty). ThreadPool is still usable after a call to this.
	
void stop()
	- Does what the name implies. Blocks until all threads have stopped. The job queue is cleared (ie: gone, will not be run), notifies running threads to terminate, wakes waiting threads so they can terminate, and then blocks until all threads have finished. Generally used when an emergency stop is required or something along those lines. Still might block for a while if currently running jobs are lengthy. If you want to wait until all queued jobs are complete, or reuse the ThreadPool later, see wait()
	
Example use can be seen in main.cpp
