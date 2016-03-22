#ifndef _UTHREAD_H
#define _UTHREAD_H

#include "uthreads.h"
#include <csetjmp>
#include <csignal>

// This is defined here so it could be used in both uthread and uthreads cpp
// files, and not only the uthreads.cpp
#define EXIT_SUCC 0
#define EXIT_FAIL -1
#define SYSTEM_ERROR 1

/*
 * Class: uthread
 * This class represents a thread in the uthreads library
 */
class uthread
{
	public:
		typedef unsigned int id;       /* thread id type */
		typedef void (*func)(void);    /* thread entry function type */
		enum state                     /* thread state enum */
		{
			READY = 0,
			RUNNING = 1,
			SLEEPING = 2,
			BLOCKED = 3
		};
		
		sigjmp_buf env;			/* The thread's context */

		/**
		 * Method: uthread
		 * Constructs a new uthread with a given id 
		 * and entry function f
		 */
		uthread (id tid, func f);

		/**
		 * Method: ~uthread
		 * uthread destructor
		 */
		~uthread ();

		/**
		 * Method: get_id
		 * Return this thread's id
		 */
		id get_id() const;


		/**
		 * Method: get_state
		 * Return this thread's state
		 */
		state get_state() const;
		
		
		/**
		 * Method: set_state
		 * Set this thread's state to a given one
		 * If the state was changed to RUNNING, returns the number
		 * of total runs of this thread (including this one).
		 * Otherwise, reutrns 0.
		 */
		void set_state(state newState);


		/**
		 * Method: get_runs
		 * Get the total number of quanta this thread ran.
		 */
		int get_runs() const;

		
		/**
		 * Method: get_wakeup
		 * Get the quantum number in which this thread needs to wake up
		 */
		int get_wakeup() const;


		/**
		 * Method: set_wakeup
		 * Set the quantum number in which this thread needs to wake up
		 */
		void set_wakeup(int quantum_num);

	private:
		const id _tid; /* Thread ID  */
		state _state; /* Thread State */
		char _stack[STACK_SIZE]; /* Thread Stack */
		int _totalRuns; /* Total num. of quanta ran by this thread */
		int _wakeUpQuantum; /* The quantum number this thread should 
				       wake up in */
		
}; /* -----  end of class uthread  ----- */

#endif     
