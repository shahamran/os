#include "uthread.h"

/**
 * Constructor for a thread object.
 * Sets up the stack pointer and program counter to the current values.
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
	sigemptyset(&env->__saved_mask);
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
int uthread::set_state(uthread::state newState)
{
	_state = newState;
	if (newState == RUNNING)
	{
		return ++_totalRuns;
	}
	return EXIT_SUCC;
}

int uthread::get_wakeup() const
{
	return _wakeUpQuantum;
}

void uthread::set_wakeup(int quantum_num)
{
	_wakeUpQuantum = quantum_num;
}
