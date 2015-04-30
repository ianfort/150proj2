#include "Thread.h"


TVMThreadID Thread::nextID = 0;

Thread::Thread()
{
  id = nextID;
  nextID++;
  stackBase = NULL;
}//default/empty constructor

Thread::Thread(const TVMThreadPriority &pri, const TVMThreadState &st, TVMThreadIDRef tid,
               uint8_t *sb, TVMMemorySize ss, const ThreadEntry &entryFunc, void *p)
{
  Thread();
  priority = pri;
  state = st;
  *tid = id;
  stackBase = sb;
  stackSize = ss;
  entry = entryFunc;
  param = p;
  ticks = -1;
  cd = -5;
}//constructor


Thread::~Thread()
{
  if (stackBase)
    delete stackBase;
}//Default destructor


SMachineContext* Thread::getContextRef()
{
  return &context;
}//Thread::SMachineContext* getContextRef()


void Thread::decrementTicks()
{
  ticks--;
  if (ticks == 0 && state == VM_THREAD_STATE_WAITING)
  {
    state = VM_THREAD_STATE_READY;
    readyQ[priority]->push(this);
  }//if no need to be asleep
  if (ticks < 0)
  {
    ticks = -5;
  }
}//void Thread::decrementTicks()


volatile int Thread::getcd()
{
  return cd;
}//volatile int Thread::getcd()


TVMThreadEntry Thread::getEntry()
{
  return entry;
}//TVMThreadEntry Thread::getEntry()


TVMThreadIDRef Thread::getIDRef()
{
  return &id;
}//TVMThreadID Thread::getIDRef()


TVMThreadPriority Thread::getPriority()
{
  return priority;
}//TVMThreadPriority Thread::getPriority()


volatile TVMThreadState Thread::getState()
{
  return state;
}//TVMThreadState Thread::getState()


volatile int Thread::getTicks()
{
  return ticks;
}//volatile int Thread::getTicks()


void Thread::setcd(volatile int calldata)
{
  cd = calldata;
}//void Thread::setcd(volatile int calldata)


void Thread::setContext(SMachineContext c)
{
  context = c;
}//void Thread::setContext(SMachineContext c)


void Thread::setID(TVMThreadID newID)
{
  id = newID;
}//Thread::TVMThreadID setID(TVMThreadID newID)


void Thread::setPriority(TVMThreadPriority pri)
{
  priority = pri;
}//void Thread::setPriority(TVMThreadPriority pri)


void Thread::setState(TVMThreadState newstate)
{
  state = newstate;
}//void Thread::setState(TVMThreadState newstate)


void Thread::setTicks(volatile int newticks)
{
  ticks = newticks;
}//void Thread::setTicks(int newticks)


