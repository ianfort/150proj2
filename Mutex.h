#ifndef MUTEX_H
#define MUTEX_H

//#include "Thread.h"
#include <queue>

// typedef unsigned int TVMMutexID, *TVMMutexIDRef;

class Mutex
{
  static TVMMutexID idCounter;
  TVMMutexID id; // the id of this mutex
  bool available; // Whether the mutex is availabe
  // queue<Thread*> *qtex; // Waiting queue for threads trying to acquire this mutex
public:
  TVMMutexID getID();
  bool getAvailable();
  
};

#endif
