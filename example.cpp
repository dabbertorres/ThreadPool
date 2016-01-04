#include <iostream>
#include <sstream>
#include <random>

#include "ThreadPool.hpp"

void hello()
{
	std::cout << "hello from thread #" << std::this_thread::get_id() << "!\n";
}

void hola()
{
	std::cout << "hola  from thread #" << std::this_thread::get_id() << "!\n";
}

int add(int x, int y)
{
	return x + y;
}

int main(int argc, char** argv)
{
	using namespace dbr;

	cc::ThreadPool pool;

	// queue up 200 jobs, then run them
	pool.pause();

	for(int i = 0; i < 200; ++i)
		pool.add(i % 2 == 1 ? hello : hola);

	pool.resume();

	// wait for all jobs to finish
	pool.wait();
	std::cout << std::endl;

	std::random_device rd;
	std::default_random_engine rand(rd());
	std::uniform_int_distribution<int> dist(1, 100);

	// get ids of threads being used
	for(auto& id : pool.ids())
		std::cout << "Thread ID: " << id << std::endl;
	std::cout << std::endl;

	std::vector<std::tuple<int, int, std::future<int>>> results;

	// add 100 more jobs of a different function type, using the same pool
	// these will start executing immediately
	for(int i = 0; i < 100; ++i)
	{
		auto one = dist(rand);
		auto two = dist(rand);

		results.emplace_back(one, two, pool.add(add, one, two));
	}

	// wait until those are all done
	pool.wait();

	for(auto& res : results)
		std::cout << std::get<0>(res) << " + " << std::get<1>(res) << " = " << std::get<2>(res).get() << std::endl;
	std::cout << std::endl;

	results.clear();

	// now add some more, using a lambda
	for(int i = 0; i < 100; ++i)
	{
		auto one = dist(rand);
		auto two = dist(rand);

		results.emplace_back(one, two, pool.add([](auto x, auto y) { return x - y; }, one, two));
	}

	// wait until those are all done
	pool.wait();

	for(auto& res : results)
		std::cout << std::get<0>(res) << " - " << std::get<1>(res) << " = " << std::get<2>(res).get() << std::endl;
	std::cout << std::endl;

	return 0;
}