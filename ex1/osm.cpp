#include "osm.h"
#include <vector>
#include <sys/time.h>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>

#define FINISH_SUCCESS 0
#define FINISH_ERROR -1
#define INVALID_ITER 0
#define DEFAULT_ITER 1000
#define MICRO_TO_NANO(x) ((x) * 1000)
#define SEC_TO_MICRO(x) ((x) * 1000000)
#define FILE_FLAGS O_CREAT | O_RDWR
#define FILE_NAME_1 "/tmp/.ransha"
#define FILE_NAME_2 "/tmp/.ransha1"
#define MAX_NAME_LEN 64

// g for global
static int gfd1 = 0, gfd2 = 0; // fd for file-descriptor
static char *gmachineName;

/* Initialization function that the user must call
 * before running any other library function.
 * The function may, for example, allocate memory or
 * create/open files.
 * Returns 0 uppon success and -1 on failure
 */
int osm_init()
{
    // Attempt to open/create two files, if one fails the other isn't opened (logical or)
    if (((gfd1 = open(FILE_NAME_1, FILE_FLAGS)) == FINISH_ERROR) ||
        ((gfd2 = open(FILE_NAME_2, FILE_FLAGS)) == FINISH_ERROR))
    {
        return FINISH_ERROR;
    }
    // Allocate memory for hostname
    gmachineName = new char[MAX_NAME_LEN];
    return FINISH_SUCCESS;
}

/* finalizer function that the user must call
 * after running any other library function.
 * The function may, for example, free memory or
 * close/delete files.
 * Returns 0 uppon success and -1 on failure
 */
int osm_finalizer()
{
    // Free allocated memory
    delete gmachineName;

    // Bitwise or to ensure both files are at least attempted to be closed
    if ((close(gfd1) == FINISH_ERROR) | (close(gfd2)) == FINISH_ERROR)
    {
        return FINISH_ERROR;
    }
    // Remove files.
    if ((unlink(FILE_NAME_1) == FINISH_ERROR) | (unlink(FILE_NAME_2) == FINISH_ERROR))
    {
        return FINISH_ERROR;
    }
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

/* Time measurement function for a simple arithmetic operation.
 * returns time in nano-seconds upon success,
 * and -1 upon failure.
 */
double osm_operation_time(unsigned int iterations)
{
    // Ensure non-zero iterations
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
        // Get random numbers
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

/* Time measurement function for an empty function call.
 * returns time in nano-seconds upon success,
 *  and -1 upon failure.
 */
double osm_function_time(unsigned int iterations)
{
    // Ensure non-zero iterations
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
        tv /= DEFAULT_ITER / 2 * 12; // 12 for number of function calls in each iteration, DEFAULT_ITER / 2 iterations
        total += tv;
    }
    return total / iterations;
}

/* Time measurement function for an empty trap into the operating system.
 * returns time in nano-seconds upon success,
 * and -1 upon failure.
 */
double osm_syscall_time(unsigned int iterations)
{
    // Ensure non-zero iterations
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
        tv /= 5; // 5 empty traps were made
        total += tv;
    }
    return total / iterations;
}

/* Time measurement function for accessing the disk.
 * returns time in nano-seconds upon success,
 * and -1 upon failure.
 */
double osm_disk_time(unsigned int iterations)
{
    // Ensure non-zero iterations
    if (iterations == INVALID_ITER)
    {
        iterations = DEFAULT_ITER;
    }
    int buffSize = 64;
    struct timeval st, et;
    double tv, total = 0;
    char oneByte[buffSize];

    for (int i = 0; i < iterations; ++i)
    {
        if (gettimeofday(&st, NULL) != 0)
            return FINISH_ERROR;
        if ((write(gfd1, oneByte, 1) == FINISH_ERROR) ||
                (write(gfd2, oneByte, 1) == FINISH_ERROR) ||
                (read(gfd1, oneByte, 1) == FINISH_ERROR) ||
                (read(gfd2, oneByte, 1) == FINISH_ERROR))
        {
            return FINISH_ERROR;
        }
        if (gettimeofday(&et, NULL) != 0)
            return FINISH_ERROR;
        tv = timeDiffInNano(et, st);
        tv /= 4; // 4 is the number of "disk access"es performed
        total += tv;
    }
    return total / iterations;
}

/*
 * Measure all times and return a struct with the relevant data.
 * Eeach field is set to -1 upon error or the correct value otherwise,
 * except machineName which is set to the null-char '\0' upon failure.
 */
timeMeasurmentStructure measureTimes(unsigned int operation_iterations,
									 unsigned int function_iterations,
									 unsigned int syscall_iterations,
									 unsigned int disk_iterations)	
{
    timeMeasurmentStructure retVal;
    // Get host name
    if (gethostname(gmachineName, MAX_NAME_LEN) != 0)
    {
        gmachineName = '\0';
    }
    retVal.machineName = gmachineName;

    // Measure times
    retVal.instructionTimeNanoSecond = osm_operation_time(operation_iterations);
    retVal.functionTimeNanoSecond = osm_function_time(function_iterations);
    retVal.trapTimeNanoSecond = osm_syscall_time(syscall_iterations);
    retVal.diskTimeNanoSecond = osm_disk_time(disk_iterations);

    // If basic op time is 0 for some reason, all ratios are invalid (divided by 0), therefore set to -1
    if (retVal.instructionTimeNanoSecond == 0 || retVal.instructionTimeNanoSecond == FINISH_ERROR)
    {
        retVal.functionInstructionRatio = retVal.trapInstructionRatio = \
                                          retVal.diskInstructionRatio = -1;
    }
    else
    {
        retVal.functionInstructionRatio = retVal.functionTimeNanoSecond / retVal.instructionTimeNanoSecond;
        retVal.trapInstructionRatio =  retVal.trapTimeNanoSecond / retVal.instructionTimeNanoSecond;
        retVal.diskInstructionRatio = retVal.diskTimeNanoSecond / retVal.instructionTimeNanoSecond;
    }
    return retVal;
}