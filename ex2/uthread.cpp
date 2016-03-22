#include "uthread.h"
#include <iostream>

#ifdef __x86_64__		    	     // ---- BLACK BOX CODE ----
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

#endif		       			// ---- END OF BLACK BOX CODE ----

/**
 * Constructor for a thread object.
 * Sets up the stack pointer and program counter to the correct values.
 * The stack is a private calss variable and the pc is the function this
 * thread starts in.
 * The thread id is determined only during construction and cannot be changed.
 */
uthread::uthread(uthread::id tid, uthread::func f) :
	_tid(tid), _state(READY), _totalRuns(0)
{
	address_t sp, pc;
	sp = (address_t)_stack + STACK_SIZE - sizeof(address_t);
	pc = (address_t)f;
	sigsetjmp(env, 1);
	(env->__jmpbuf)[JB_SP] = translate_address(sp);
	(env->__jmpbuf)[JB_PC] = translate_address(pc);
	if (sigemptyset(&env->__saved_mask) < 0)
	{
		std::cerr << "system error: sigemptyset failed" << std::endl;
		exit(SYSTEM_ERROR);
	}
}

/**
 * Empty destructor
 */
uthread::~uthread()
{
}

/**
 * Return the thread id
 */
uthread::id uthread::get_id() const
{
	return _tid;
}

/**
 * Return the thread state
 */
uthread::state uthread::get_state() const
{
	return _state;
}

/**
 * Set the thread's state to a given state.
 * If the new state is RUNNING, the runs count of this thread is
 * incremented.
 */
void uthread::set_state(uthread::state newState)
{
	_state = newState;
	if (newState == RUNNING)
	{
		++_totalRuns;
	}
}

/**
 * Return the total number of qunata this thread ran
 */
int uthread::get_runs() const
{
	return _totalRuns;
}

/**
 * Return the qunatum number in which this tread needs to wake up.
 */
int uthread::get_wakeup() const
{
	return _wakeUpQuantum;
}

/**
 * Set the quantum number in which this thread needs to wake up.
 */
void uthread::set_wakeup(int quantum_num)
{
	_wakeUpQuantum = quantum_num;
}
