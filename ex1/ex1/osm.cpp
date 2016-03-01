#include "osm.h"
#include <vector>
#include <ctime>

#define RETURN_SUCCESS 0
#define RETURN_ERROR -1
#define INVALID_ITER 0
#define DEFAULT_ITER 1000
#define MICRO_TO_NANO(x) ((x) * 1000);

int osm_init()
{

}

int osm_finalizer()
{

}

double osm_operation_time(unsigned int iterations)
{
	if (iterations == INVALID_ITER)
	{
		iterations = DEFAULT_ITER;
	}

	srand(time(NULL));
	int a, b, c, d, j;
	double tv, total = 0;

	for (int i = 0; i < iterations; ++i)
	{
		a = rand();
		b = rand();
		c = rand();
		d = rand();
		// Start measure
		for (j = 0; j < DEFAULT_ITER; ++j)
		{
			a + b;
			a & b;
			a | b;
			a + c;
			a & c;
			a | c;
			b + d;
			b & d;
			b | d;
			b + c;
			b & c;
			b | c;
			a + d;
			a & d;
			a | d;
		}
		// End measure
		tv /= DEFAULT_ITER * 15;
		total += MICRO_TO_NANO(tv) /* measured time */;
	}
	return total / iterations;
}

double osm_function_time(unsigned int iterations)
{

}

double osm_syscall_time(unsigned int iterations)
{

}

double osm_disk_time(unsigned int iterations)
{

}

timeMeasurmentStructure measureTimes(unsigned int operation_iterations,
									 unsigned int function_iterations,
									 unsigned int syscall_iterations,
									 unsigned int disk_iterations)	
{

}