#include "uthreads.h"
#include "uthread.h"            // Note that this is the uthread object class
#include <sys/time.h>
#include <csignal>
#include <csetjmp>
#include <iostream>
#include <map>                  // For the threads list
#include <list>			// For the READY threads list
#include <algorithm>

#define MAIN_THREAD_ID 0
#define MAIN_THREAD_FUNC nullptr // Assuming (address_t)nullptr == 0
#define SAVE_SIGS 1
#define LONGJMP_VAL 1
#define SHOULD_WAKE 0
#define SIG_FLAGS 0

/* Counts the total number of quanta ran */
int totalQuanta = 0;
/* Holds the current running thread's id */
uthread::id currentThread = MAIN_THREAD_ID;
/* Holds all threads that were spawned and weren't terminated */
std::map<uthread::id, uthread*> livingThreads;
/* Holds all threads in the state READY */
std::list<uthread::id> readyThreads;
/* Holds the thread that needs to be deleted but couldn't */
uthread::id toDelete;


/**
 * Function: setTimer
 * Sets the timer to the given quantum_usecs value.
 * Returns the setitimer function result, i.e. 0 upon success and
 * negative number upon failure.
 */
int setTimer(int quantum_usecs)
{
	struct itimerval timer;
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_value.tv_usec = quantum_usecs;
	timer.it_interval.tv_usec = quantum_usecs;
	return setitimer (ITIMER_VIRTUAL, &timer, NULL);
}


/**
 * Function: resetTimer
 * Resets the timer to 'quantum_usecs'
 * Returns 0 upon success and -1 upon failure.
 * (Assumes current timer interval is set to 'quantum_usecs')
 */
int resetTimer()
{
	struct itimerval timer;
	// Get the value of the timer from the interval value
	// (supposed to be initialized with 'quantum_usecs')
	if (getitimer(ITIMER_VIRTUAL, &timer) < 0)
	{
		std::cerr << "getitimer error." << std::endl;
		return EXIT_FAIL;
	}
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_value.tv_usec = timer.it_interval.tv_usec;
	
	// Reset the value of the timer
	if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
	{
		std::cerr << "setitimer error." << std::endl;
		return EXIT_FAIL;
	}
	return EXIT_SUCC;
}


/**
 * Function: deleteThread
 * Deletes a thread from the living threads list, the READY list (if it's
 * there) and frees its allocated memory.
 * Assumes the given tid exists in the living threads list.
 * Calling with tid == MAIN_THREAD_ID does nothing.
 * DO NOT CALL WITH UNEXISTING THREAD ID
 */
void deleteThread(uthread::id tid)
{
	if (tid == MAIN_THREAD_ID)
	{
		return;
	}	
	uthread *curr = livingThreads[tid];
	livingThreads.erase(tid);
	auto th = std::find(readyThreads.begin(),
			    readyThreads.end(), tid);
	if (th != readyThreads.end())
	{
		readyThreads.erase(th);
	}
	delete curr;
	return;
}


/**
 * Function: wakeSleepingThreads
 * Iterates over all threads alive and wakes them up if needed.
 * If a thread is awaken it is added to the READY list.
 */
void wakeSleepingThreads()
{
	uthread *curr;
	// Add waking threads to READY list
	for (auto th=livingThreads.cbegin(); th != livingThreads.cend(); ++th)
	{
		// curr is the current thread in the for loop
		curr = th->second;
		// If the thread is sleeping and it should wake up, wake it up
		if (curr->get_state() == uthread::state::SLEEPING &&
		    uthread_get_time_until_wakeup(curr->get_id())==SHOULD_WAKE)
		{
			curr->set_state(uthread::state::READY);
			readyThreads.push_back(curr->get_id());
		}
	}
}


/**
 * The scheduling function for this library.
 * In charge of switching between threads, incrementing the quanta count and
 * maintaining the READY list.
 * This function returns only when the next thread starts running.
 */
void contextSwitch(int)
{
	// Useful current thread pointer
	// everywhere in this function, curr holds a pointer to the currently
	// discussed thread, and it changes throughout the function.
	uthread *curr = livingThreads[currentThread];

	// Save current thread's environment
	int ret_val = sigsetjmp(curr->env, SAVE_SIGS);
	// If you came here from siglongjmp, go away!
	if (ret_val == LONGJMP_VAL)
	{
		// If the last thread tried to terminate itself,
		// Finish the job. Now.
		deleteThread(toDelete);
		toDelete = MAIN_THREAD_ID;
		return;
	}

	// Block SIGTVALRM signal
	sigset_t set, oldSet;
	sigemptyset(&set);
	sigaddset(&set, SIGVTALRM);
	sigprocmask(SIG_BLOCK, &set, &oldSet);
	
	// Increment quanta count
	++totalQuanta;

	// Wake sleeping threads
	wakeSleepingThreads();

	// If the thread stopped because of quantum end,
	// add it to the READY list
	if (curr->get_state() == uthread::state::RUNNING)
	{
		curr->set_state(uthread::state::READY);
		readyThreads.push_back(currentThread);
	}

	// Reset Timer
	resetTimer();

	// Switch to the next READY thread
	currentThread = readyThreads.front();
	readyThreads.pop_front();
	curr = livingThreads[currentThread];
	curr->set_state(uthread::state::RUNNING);
	// Note that the saved env for each thread does not block
	// SIGVTALRM, therefore no signal unblocking is needed here.
	siglongjmp(curr->env, LONGJMP_VAL);
}


/*
 * Description: This function initializes the thread library. 
 * Call before any other thread library function, and exactly once. 
 * The input to the function is the length of a quantum in micro-seconds. 
 * It is an error to call this function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
	// Block SIGTVALRM signal
	sigset_t set, oldSet;
	sigemptyset(&set);
	sigaddset(&set, SIGVTALRM);
	sigprocmask(SIG_BLOCK, &set, &oldSet);
	
	struct sigaction sa;

	// Make sure the quantum time is positive.	
	if (quantum_usecs <= 0) 
	{
		return EXIT_FAIL;
	}

	// Set up a the context switch func. as SIGVTALRM handler
	sa.sa_handler = &contextSwitch;
	sa.sa_flags = SIG_FLAGS;
	if (sigaction(SIGVTALRM, &sa, NULL) < 0) {
		std::cerr << "sigaction error." << std::endl;
		return EXIT_FAIL;
	}

	// Create the main thread
	uthread *mainThread = new uthread(MAIN_THREAD_ID, MAIN_THREAD_FUNC);
	mainThread->set_state(uthread::state::RUNNING);
	++totalQuanta;
	// Add main's id to live threads list (Default == 0).
	livingThreads.insert(std::make_pair(mainThread->get_id(), mainThread));

	// Set up a timer
	if (setTimer(quantum_usecs) < 0)
	{
		std::cerr << "setitimer error." << std::endl;
		return EXIT_FAIL; /* If a timer setting failed,
					the function had failed. */
	}

	// Unblock SIGVTALRM and return
	sigprocmask(SIG_SETMASK, &oldSet, NULL);
	return EXIT_SUCC;
}


/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
	// If the current number of threads is MAX_THREAD_NUM, no
	// more threads can be spawned.
	if (livingThreads.size() >= MAX_THREAD_NUM)
	{
		return EXIT_FAIL;
	}

	// Find the smallest id available, starting from MAIN_ID + 1.
	uthread::id newId = MAIN_THREAD_ID + 1;
	for (auto it = ++livingThreads.cbegin(); /* start from the second */
		  it != livingThreads.cend();
		++it)
	{
		if (newId++ < it->first)
		{
			// If the threads ids numbering doesn't match natural
			// incremental numbering, we found an empty spot.
			break;
		}
	}

	// Create a fresh thread with the found id and entry function f
	// Then put it in the living threads list and READY list.
	uthread *newThread = new uthread(newId, f);
	livingThreads.insert(std::make_pair(newId, newThread));
	readyThreads.push_back(newId);
	return newId;
}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered as an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory]. 
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
	// If main thread is terminated, delete all threads and quit.
	// The deleteThread function checks and manages data for future
	// process run, which is un-needed in this point.
	if (tid == MAIN_THREAD_ID)
	{
		for (auto th : livingThreads)
		{
			delete th.second;
		}
		exit(EXIT_SUCC);
	}
	// If no such thread exists, return failure.
	if (livingThreads.find(tid) == livingThreads.end())
	{
		std::cerr << "error terminating thread: " << tid 
			  << " - no such thread id" << std::endl;
		return EXIT_FAIL;
	}
	// Otherwise, delete thread tid, if it's not sleeping.
	if (livingThreads[tid]->get_state() == uthread::state::SLEEPING)
	{
		std::cerr << "wrong library usage - "
		       << "can't terminate a sleeping thread" << std::endl;
		return EXIT_FAIL;
	}
	// If deleted yourself, goto contextSwitch.
	if ((uthread::id)tid == currentThread)
	{
		toDelete = tid;
//		threadsToDelete.insert(std::make_pair(tid, livingThreads[tid]));
		contextSwitch(SIGVTALRM);
		return EXIT_SUCC; // This should not be reached
	}
	// Otherwise, delete and return
	else
	{
		deleteThread(tid);
		return EXIT_SUCC;
	}
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED or SLEEPING states has no
 * effect and is not considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
	// Blocking the main thread is an error
	if (tid == MAIN_THREAD_ID)
	{
		std::cerr << "error - can't block main thread" << std::endl;
		return EXIT_FAIL;
	}
	// If no such thread exists, it is an error
	if (livingThreads.find(tid) == livingThreads.end())
	{
		std::cerr << "error blocking thread: " << tid
			<< " - no such thread id" << std::endl;
		return EXIT_FAIL;
	}
	// Check if the thread CAN be blocked
	// if not, do nothing.
	uthread *curr = livingThreads[tid];
	if (curr->get_state() == uthread::state::BLOCKED ||
	    curr->get_state() == uthread::state::SLEEPING)
	{
		return EXIT_SUCC;
	}
	// If a thread blocks itself switch context and exit
	// (the return will be executed after the thread is resumed)
	if ((uthread::id)tid == currentThread)
	{
		curr->set_state(uthread::state::BLOCKED);
		contextSwitch(0);
		return EXIT_SUCC;
	}
	// Otherwise, change the thread's state to BLOCKED and remove it
	// from the READY list.
	curr->set_state(uthread::state::BLOCKED);
	auto th = std::find(readyThreads.begin(), readyThreads.end(), tid);
	readyThreads.erase(th);
	return EXIT_SUCC;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in the RUNNING, READY or SLEEPING
 * state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
	if (livingThreads.find(tid) == livingThreads.end())
	{
		std::cerr << "error resuming thread: " << tid
			<< " - no such thread id" << std::endl;
		return EXIT_FAIL;
	}
	uthread *curr = livingThreads[tid];
	if (curr->get_state() == uthread::state::BLOCKED)
	{
		curr->set_state(uthread::state::READY);
		readyThreads.push_back(curr->get_id());
	}
	return EXIT_SUCC;
}


/*
 * Description: This function puts the RUNNING thread to sleep for a period
 * of num_quantums (not including the current quantum) after which it is moved
 * to the READY state. num_quantums must be a positive number. It is an error
 * to try to put the main thread (tid==0) to sleep. Immediately after a thread
 * transitions to the SLEEPING state a scheduling decision should be made.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums)
{
	if (currentThread == MAIN_THREAD_ID)
	{
		// ERRORRR
	}
	uthread *curr = livingThreads[currentThread];
	curr->set_wakeup(totalQuanta + num_quantums);
	curr->set_state(uthread::state::SLEEPING);
	contextSwitch(SIGVTALRM);
	return EXIT_SUCC;
}


/*
 * Description: This function returns the number of quantums until the thread
 * with id tid wakes up including the current quantum. If no thread with ID
 * tid exists it is considered as an error. If the thread is not sleeping,
 * the function should return 0.
 * Return value: Number of quantums (including current quantum) until wakeup.
*/
int uthread_get_time_until_wakeup(int tid)
{
	// If no such thread exists, this is an error
	if (livingThreads.find(tid) == livingThreads.end())
	{
		//ERRORRR
		return EXIT_FAIL;
	}
	else if (livingThreads[tid]->get_state() != uthread::state::SLEEPING)
	{
		// If the thread is not sleeping, return 0.
		return 0;
	}
	else
	{
		// In general, diff shouldn't be negative, but if it does
		// it shouldn't crash the whole program.
		int diff = livingThreads[tid]->get_wakeup() - totalQuanta;
		return diff >= 0 ? diff : 0;
	}
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
	return currentThread;
}


/*
 * Description: This function returns the total number of quantums that were
 * started since the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
	return totalQuanta;
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered as an error.
 * Return value: On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
	if (livingThreads.find(tid) == livingThreads.end())
	{
		// ERRRORRR
	}
	return livingThreads[tid]->get_runs();
}
