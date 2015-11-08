#include <iostream>
#include <sstream>
#include <string>
#include <random>

#include "ThreadPool.hpp"

void function()
{
	auto id = std::this_thread::get_id();

	std::ostringstream oss;
	oss << "hello from thread #: ";
	oss << id;
	oss << "!\n";
	
	std::cerr << oss.str();
}

void function2()
{
	auto id = std::this_thread::get_id();

	std::ostringstream oss;
	oss << "goodbye from thread #: ";
	oss << id;
	oss << "!\n";

	std::cerr << oss.str();
}

int add(int x, int y)
{
	return x + y;
}

int main(int argc, char** argv)
{
	{
		cc::ThreadPool<void()> pool;

		for(int i = 0; i < 100; ++i)
			pool.add(function);

		pool.wait();

		for(int i = 0; i < 100; ++i)
			pool.add(function2);

		pool.wait();
	}

	{
		std::random_device rd;
		std::default_random_engine rand(rd());
		std::uniform_int_distribution<int> dist(1, 100);

		cc::ThreadPool<int(int, int)> pool;
		std::vector<std::pair<int, int>> args;
		std::vector<std::future<int>> returns;

		for(int i = 0; i < 100; ++i)
		{
			args.emplace_back(dist(rand), dist(rand));
			returns.emplace_back(pool.add(add, args.back().first, args.back().second));
		}

		pool.wait();

		for(int i = 0; i < 100; ++i)
			std::cout << args[i].first << " + " << args[i].second << " = " << returns[i].get() << '\n';
	}

	return 0;
}