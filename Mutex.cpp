#include "Mutex.h"

TVMMutexID Mutex::idCounter = 0;


Mutex::Mutex()
{
  id = idCounter;
  idCounter++;
}


TVMMutexID Mutex::getID()
{
}


bool Mutex::getAvailable()
{
  if (owner)
  {
    return true;
  }

  return false;
}


int Mutex::acquire(Thread* thrd, TVMTick timeout)
{
  // Return value for debugging

  if (owner)
  {
    if ( !isInQueue(thrd->getID()) && timeout != VM_TIMEOUT_IMMEDIATE)
    {
      QTex->push(thrd)
      return ACQUIRE_WAIT;
    }
    return ACQUIRE_UNNECESSARY;
  }
  owner = thrd;
  
  return ACQUIRE_SUCCESS;
}


bool Mutex::release();
{
  // Return value for debugging

  if (owner)
  {
    owner = NULL;
    return true;
  }

  return false;
}


bool isInQueue(TVMThreadID id)
{
  for (queue<Thread*>::iterator itr = QTex->begin() ; itr != QTex->end() ; itr++)
  {
    if ( (*itr)->getID() == id )
    {
      return true;
    }
  }

  return false;
}

