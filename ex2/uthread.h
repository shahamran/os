#ifndef  _UTHREAD_H
#define _UTHREAD_H

#include "uthreads.h"
#include <csetjmp>

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
		"rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5 

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

/*
 * ==========================================================================
 *        Class:  uthread
 *  Description:  This class represents a thread in the uthreads library
 * ===========================================================================
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

		/**
		 * Method: uthread
		 * Constructs a new uthread with a given id (def. is main==0)
		 * and entry function f (def. is ?)
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
		id get_id();

		/**
		 * Method: get_state
		 * Return this thread's state
		 */
		state get_state();
	private:
		sigjmp_buf _env;
		id _tid;
		state _state;
		char _stack[STACK_SIZE];
		void (*_func)(void)
		
}; /* -----  end of class uthread  ----- */

#endif     
