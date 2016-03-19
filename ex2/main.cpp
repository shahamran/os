#include <iostream>
#include "uthreads.h"
#include <unistd.h>

#define FINISH_ERROR -1
#define FINISH_SUCC 0

void f(void)
{
	while (1);
}

void g(void)
{
	while(1);
}

int main()
{
	int quantum_usecs;
	std::cout << "Enter thread interval:" << std::endl;
	std::cin >> quantum_usecs;
	int ret_val = uthread_init(quantum_usecs);
	if (ret_val < 0)
	{
		std::cerr << "init failed with return code:" << ret_val << std::endl;
		return FINISH_ERROR;
	}
	if ((ret_val = uthread_spawn(f)) < 0)
	{
		std::cerr << "spawn failed" << std::endl;
	}
	std::cout << "spawned a new thread, number: " << ret_val <<std::endl;
	g();
	if ((ret_val = uthread_terminate(0)) < 0)
	{
		std::cerr << "terminate main failed with code:" << ret_val << std::endl;
		return FINISH_ERROR;
	}
	std::cout << "Finished without errors" << std::endl;
	return FINISH_SUCC;
}
