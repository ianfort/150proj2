#include "Mutex.h"

TVMMutexID Mutex::idCounter = 0;

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


bool Mutex::acquire()
{
  // Return value for debugging

  if (!available)
  {
    available = true;
    return true;
  }

    return false;
}


bool Mutex::release();
{
  // Return value for debugging

  if (available)
  {
    available = false;
    return true;
  }

  return false;
}

