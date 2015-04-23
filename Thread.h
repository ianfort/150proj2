#ifndef THREAD_H
#define THREAD_H

#include <stack>

// extern "C"
// {


class Thread
{
  TVMThreadPriority priority;
  TVMThreadState state;
  TVMThreadID id;
  stack<void*> TStack;
  int ticksLeft;
  // void* entryFunc
  void* entryPeram;
};


// }


#endif
