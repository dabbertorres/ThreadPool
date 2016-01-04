#include <sstream>
#include <random>

#include "ThreadPool.hpp"

void function()
{
	std::ostringstream oss;
	oss << std::this_thread::get_id();
	auto id = oss.str();
	std::printf("hello from thread #%s!\n", id.c_str());
}

void function2()
{
	static volatile std::size_t calls = 0;
	std::ostringstream oss;
	oss << std::this_thread::get_id();
	auto id = oss.str();
	std::printf("hola  from thread #%s!\n", id.c_str());
}

int add(int x, int y)
{
	return x + y;
}

int main(int argc, char** argv)
{
	using namespace dbr;

	{
		cc::ThreadPool pool;

		pool.pause();

		for(int i = 0; i < 200; ++i)
			pool.add(i % 2 == 1 ? &function : &function2);

		pool.resume();
		pool.wait();
		std::printf("\n");
	}
	
	{
		std::random_device rd;
		std::default_random_engine rand(rd());
		std::uniform_int_distribution<int> dist(1, 100);

		cc::ThreadPool pool;
		std::vector<std::tuple<int, int, std::future<int>>> results;

		for(int i = 0; i < 100; ++i)
		{
			auto one = dist(rand);
			auto two = dist(rand);

			results.emplace_back(one, two, pool.add(&add, one, two));
		}

		pool.wait();

		for(auto& res : results)
			std::printf("%d + %d = %d\n", std::get<0>(res), std::get<1>(res), std::get<2>(res).get());
		std::printf("\n");

		results.clear();
		for(int i = 0; i < 100; ++i)
		{
			auto one = dist(rand);
			auto two = dist(rand);

			results.emplace_back(one, two, pool.add([](auto x, auto y) { return x - y; }, one, two));
		}

		pool.wait();

		for(auto& res : results)
			std::printf("%d - %d = %d\n", std::get<0>(res), std::get<1>(res), std::get<2>(res).get());
	}

	return 0;
}