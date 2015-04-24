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
//  int ticksLeft;
public:
//  Thread();
//  Thread~();
  SMachineContext* getContextRef();
  void incrementTicks();
  void resetTicks();
  void setID(TVMThreadID newID);
  void setPriority(TVMThreadPriority pri);
};


// }


#endif
