#include "uthread.h"

uthread::uthread(uthread::id tid, uthread::func f) :
	_tid(tid), _func(f), _state(READY)
{
	address_t sp, pc;
	sp = (address_t)_stack + STACK_SIZE - sizeof(address_t);
	pc = (address_t)_func;
	sigsetjmp(_env, 1);
	(_env->__jmpbuf)[JB_SP] = translate_address(sp);
	(_env->__jmpbuf)[JB_PC] = translate_address(pc);
	sigemptyset(&_env->__saved_mask);
}

