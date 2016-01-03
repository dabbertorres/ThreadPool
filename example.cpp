#include <iostream>
#include <sstream>
#include <string>
#include <random>

#include "ThreadPool.hpp"

void function()
{
	auto id = std::this_thread::get_id();

	std::ostringstream oss;
	oss << "hello from thread #";
	oss << id;
	oss << "!\n";
	
	std::cerr << oss.str();
}

void function2()
{
	auto id = std::this_thread::get_id();

	std::ostringstream oss;
	oss << "hola  from thread #";
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

		for(int i = 0; i < 200; ++i)
			pool.add(i % 2 == 1 ? function : function2);

		pool.wait();
	}

	{
		std::random_device rd;
		std::default_random_engine rand(rd());
		std::uniform_int_distribution<int> dist(1, 100);

		cc::ThreadPool<int(int, int)> pool;
		std::vector<std::tuple<int, int, std::future<int>>> results;

		for(int i = 0; i < 100; ++i)
		{
			auto one = dist(rand);
			auto two = dist(rand);

			results.emplace_back(one, two, pool.add(add, one, two));
		}

		pool.wait();

		for(auto& res : results)
			std::cout << std::get<0>(res) << " + " << std::get<1>(res) << " = " << std::get<2>(res).get() << '\n';
	}

	return 0;
}