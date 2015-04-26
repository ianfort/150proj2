#ifndef THREAD_H
#define THREAD_H

#include <stack>

// extern "C"
// {

typedef void (*ThreadEntry)(void *param);

class Thread
{
  SMachineContext context;
  TVMThreadPriority priority;
  TVMThreadState state;
  TVMThreadID id;
  volatile unsigned int ticks;
  volatile int cd; //calldata
//  stack<void*> TStack;
//entry point
//entry params
public:
//  Thread();
//  Thread~();
  SMachineContext* getContextRef();
  void decrementTicks();
  TVMThreadPriority getPriority();
  TVMThreadState getState();
  void setID(TVMThreadID newID);
  void setPriority(TVMThreadPriority pri);
  void setState(TVMThreadState newstate);
  void setTicks(unsigned int newticks);
};


// }


#endif
