#include "Thread.h"


/*
  SMachineContext context;
  SMachineContextRef contextRef;
  TVMThreadPriority priority;
  TVMThreadState state;
  TVMThreadID id;
  stack<void*> TStack;
  int ticksLeft;
  ThreadEntry entryFunc
  void* entryParam;
*/

/*
Thread::Thread(const SMachineContext &ctxt, const SMachineContextRef contextRef, const TVMThreadPriority &priority,
		const TVMThreadState &state, const TVMThreadID &id, const stack<void*> &TStack, const int &ticksLeft,
		const  ThreadEntry &entryFunc, )
{
  context 
}


Thread::Thread~()
{
}
*/

SMachineContext* Thread::getContextRef()
{
  return &context;
}//Thread::SMachineContext* getContextRef()


void Thread::incrementTicks()
{
  ticks++;
}//void Thread::incrementTicks()


void Thread::resetTicks()
{
  ticks = 0;
}//void Thread::resetTicks()


void Thread::setID(TVMThreadID newID)
{
  id = newID;
}//Thread::TVMThreadID setID(TVMThreadID newID)


void Thread::setPriority(TVMThreadPriority pri)
{
  priority = pri;
}//void Thread::setPriority(TVMThreadPriority pri)



