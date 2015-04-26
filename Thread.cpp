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


void Thread::decrementTicks()
{
  ticks--;
  if (ticks <= 0 && state == VM_THREAD_STATE_WAITING)
  {
    state = VM_THREAD_STATE_READY;
    //insert in correct ready queue here
  }
}//void Thread::decrementTicks()


TVMThreadPriority getPriority()
{
  return priority;
}//TVMThreadPriority getPriority()


TVMThreadState getState()
{
  return state;
}//TVMThreadState getState()


void Thread::setID(TVMThreadID newID)
{
  id = newID;
}//Thread::TVMThreadID setID(TVMThreadID newID)


void Thread::setPriority(TVMThreadPriority pri)
{
  priority = pri;
}//void Thread::setPriority(TVMThreadPriority pri)


void setState(TVMThreadState newstate)
{
  state = newstate;
}//void setState(TVMThreadState newstate)


void Thread::setTicks(unsigned int newticks)
{
  ticks = newticks;
}//void Thread::setTicks(unsigned int newticks)


