#include "osm.h"
#include <vector>
#include <sys/time.h>
#include <cstdlib>
#include <unistd.h>

#define FINISH_SUCCESS 0
#define FINISH_ERROR -1
#define INVALID_ITER 0
#define DEFAULT_ITER 1000
#define MICRO_TO_NANO(x) ((x) * 1000)
#define SEC_TO_MICRO(x) ((x) * 1000000)

int osm_init()
{
    return FINISH_SUCCESS;
}

int osm_finalizer()
{
    return FINISH_SUCCESS;
}

/*
 * Empty functions for the function-call time measurements.
 */
void emptyFunc() {}
void emptyFunc2() {}

/*
 * Subtracts the second timeval struct from the first to get the difference between them in microseconds,
 * then converts to nano-seconds.
 */
double timeDiffInNano(timeval a, timeval b)
{
    return MICRO_TO_NANO(SEC_TO_MICRO(a.tv_sec - b.tv_sec) + a.tv_usec - b.tv_usec);
}

double osm_operation_time(unsigned int iterations)
{
	if (iterations == INVALID_ITER)
	{
		iterations = DEFAULT_ITER;
	}

	std::srand(0);
	int a, b, c, d, j;
    struct timeval st, et;
	double tv, total = 0;

	for (int i = 0; i < iterations; ++i)
	{
		a = std::rand();
		b = std::rand();
		c = std::rand();
		d = std::rand();
		if (gettimeofday(&st, NULL) != 0) // Start measure
            return FINISH_ERROR;
        /* Do a lot of basic operations on random numbers */
		for (j = 0; j < DEFAULT_ITER; ++j) {
            //
            a + b;
            a & b;
            a | b;
            b + d;
            b & d;
            b | d;
            a + c;
            a & c;
            a | c;
            b + c;
            b & c;
            b | c;
            a + d;
            a & d;
            a | d;
        }
        if (gettimeofday(&et, NULL) != 0) // End measure
            return FINISH_ERROR;
        // Calculate the time took (and convert to double)
        tv = timeDiffInNano(et, st);
		tv /= DEFAULT_ITER * 15; // 15 is the number of operations in each inner loop iteration
		total += tv; // accumulative measured time
	}
	return total / iterations; // Average time
}

double osm_function_time(unsigned int iterations)
{
    if (iterations == INVALID_ITER)
    {
        iterations = DEFAULT_ITER;
    }

    int j;
    struct timeval st, et;
    double tv, total = 0;

    for (int i = 0; i < iterations; ++i)
    {
        if (gettimeofday(&st, NULL) != 0)
            return FINISH_ERROR;
        for (j = 0; j < DEFAULT_ITER / 2; ++j)
        {
            emptyFunc();
            emptyFunc2();
            emptyFunc();
            emptyFunc2();
            emptyFunc();
            emptyFunc2();
            emptyFunc();
            emptyFunc2();
            emptyFunc();
            emptyFunc2();
            emptyFunc();
            emptyFunc2();
        }
        if (gettimeofday(&et, NULL) != 0)
            return FINISH_ERROR;
        tv = timeDiffInNano(et, st);
        tv /= DEFAULT_ITER / 2 * 12;
        total += tv;
    }
    return total / iterations;
}

double osm_syscall_time(unsigned int iterations)
{
    if (iterations == INVALID_ITER)
    {
        iterations = DEFAULT_ITER;
    }

    struct timeval st, et;
    double tv, total = 0;

    for (int i = 0; i < iterations; ++i)
    {
        if (gettimeofday(&st, NULL) != 0)
            return FINISH_ERROR;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        if (gettimeofday(&et, NULL) != 0)
            return FINISH_ERROR;
        tv = timeDiffInNano(et, st);
        tv /= 5;
        total += tv;
    }
    return total / iterations;
}

double osm_disk_time(unsigned int iterations)
{

}

timeMeasurmentStructure measureTimes(unsigned int operation_iterations,
									 unsigned int function_iterations,
									 unsigned int syscall_iterations,
									 unsigned int disk_iterations)	
{


    double opTime, funcTime, trapTime, diskTime, funcOp, trapOp, diskOp;

    opTime = osm_operation_time(operation_iterations);
    funcTime = osm_function_time(function_iterations);
    trapTime = osm_syscall_time(syscall_iterations);
    diskTime = osm_disk_time(disk_iterations);
    if (!opTime)
    {
        funcOp = trapOp = diskOp = -1;
    }
    else
    {
        funcOp = funcTime / opTime;
        trapOp = trapTime / opTime;
        diskOp = diskTime / opTime;
    }
    timeMeasurmentStructure retVal;
}