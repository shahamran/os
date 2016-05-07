/**
 * This header is a wrapper for some of the pthread library functions
 * which adds error handling, as specified in the exercise instructions.
 * All functions don't return any value (void functions) and upon errors they
 * print the default error message and exit the process.
 */
#ifndef _MY_PTHREAD_H
#define _MY_PTHREAD_H

#include <pthread.h>
#include <iostream>

using std::cerr;
using std::endl;

#define PTHREAD_SUCCESS 0
#define EXIT_FAIL 1
#define DEF_ERROR_MSG(FUNC_NAME) "MapReduceFramework Failure: " FUNC_NAME \
	" failed."

void my_pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
		void *(*start_routine) (void *), void *arg)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_create(thread, attr, start_routine, arg);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_create") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_join(pthread_t thread, void **retval)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_join(thread, retval);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_join") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_mutex_init(pthread_mutex_t *mutex, 
    const pthread_mutexattr_t *attr)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_mutex_init(mutex, attr);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_mutex_init") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_mutex_destroy(mutex);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_mutex_destroy") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_mutex_lock(pthread_mutex_t *mutex)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_mutex_lock(mutex);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_mutex_lock") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_mutex_unlock(mutex);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_mutex_unlock") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_cond_init(pthread_cond_t *cond, 
		const pthread_condattr_t *attr)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_cond_init(cond, attr);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_cond_init") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_cond_destroy(pthread_cond_t *cond)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_cond_destroy(cond);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_cond_destroy") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_cond_signal(pthread_cond_t *cond)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_cond_signal(cond);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_cond_signal") << endl;
		exit(EXIT_FAIL);
	}
}

void my_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_cond_wait(cond, mutex);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_cond_wait") << endl;
		exit(EXIT_FAIL);
	}

}

void my_pthread_cond_timedwait(pthread_cond_t *cond,
		pthread_mutex_t *mutex, const struct timespec *abstime)
{
	int ret_code = PTHREAD_SUCCESS;
	ret_code = pthread_cond_timedwait(cond, mutex, abstime);
	if (ret_code != PTHREAD_SUCCESS)
	{
		cerr << DEF_ERROR_MSG("pthread_cond_timedwait") << endl;
		exit(EXIT_FAIL);
	}

}

#endif

