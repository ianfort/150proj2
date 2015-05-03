#ifndef MUTEX_H
#define MUTEX_H

//#include "Thread.h"
#include "VirtualMachine.h"
#include <queue>

// typedef unsigned int TVMMutexID, *TVMMutexIDRef;

class Thread;

class Mutex
{
  static TVMMutexID idCounter;
  TVMMutexID id; // the id of this mutex
//  bool available; // Whether the mutex is availabe
  queue<Thread*> *QTex;
  Thread* owner;
  // queue<Thread*> *qtex; // Waiting queue for threads trying to acquire this mutex
public:
  TVMMutexID getID();
  bool getAvailable();
  bool acquire();
  bool release();

  void popQTex()
  void pushQTex()
  void 
};

#endif
