#ifndef THREAD_H
#define THREAD_H

#include "VirtualMachine.h"
#include "Machine.h"
#include <stack>

using namespace std;

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
  stack<void*> *tStack;
  TVMThreadEntry entry;
  void *param;
public:
  volatile int cd; //calldata. YES THIS IS PUBLIC.
  Thread();
  Thread(const TVMThreadPriority &pri, const TVMThreadState &st, const TVMThreadID &tid, stack<void*> *ts,
         const ThreadEntry &entryFunc, void *p);
  ~Thread();
  SMachineContext* getContextRef();
  void decrementTicks();
//  volatile int getcd();
  TVMThreadIDRef getIDRef();
  TVMThreadPriority getPriority();
  TVMThreadState getState();
//  void setcd(volatile int calldata);
  void setContext(SMachineContext c);
  void setID(TVMThreadID newID);
  void setPriority(TVMThreadPriority pri);
  void setState(TVMThreadState newstate);
  void setTicks(volatile unsigned int newticks);
};


// }


#endif
