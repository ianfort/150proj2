#include "VirtualMachine.h"
#include "Machine.h"
#include <unistd.h>
#include <iostream>
#include <vector>
#include "Thread.h"

using namespace std;

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);
void fileCallback(void* calldata, int result);
void incrementTicks(void*);

volatile vector<Thread*> *threads;

TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[])
{ 
  ticks = 0;
  threads = new vector<Thread*>;
  
  TVMMainEntry mainFunc = VMLoadModule(argv[0]);
  MachineInitialize(machinetickms);
  MachineRequestAlarm((tickms*1000), incrementTicks, NULL);
  MachineEnableSignals();
  if (mainFunc)
  {
    mainFunc(argc, argv);
    VMUnloadModule();
    MachineTerminate();
    delete threads;
    return VM_STATUS_SUCCESS;
  }
  MachineTerminate();
  delete threads;
  return VM_STATUS_FAILURE;
} //VMStart


TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor)
{
  cd = -2; //impossible to have a negative file descriptor
  if (filename == NULL || filedescriptor == NULL)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
  //change thread state to VM_THREAD_STATE_WAITING
  MachineFileOpen(filename, flags, mode, fileCallback, (void*)&cd);
  // void MachineFileOpen(const char *filename, int flags, int mode, TMachineFileCallback callback, void *calldata);
  while((cd) < 0); //wait until file is actually open
  *filedescriptor = cd;
  //change thread state to VM_THREAD_STATE_READY
  return VM_STATUS_SUCCESS;
//  return VM_STATUS_FAILURE;
} // VMFileOpen


TVMStatus VMFileClose(int filedescriptor)
{
  cd = 1;
  // Make thread wait here.
  MachineFileClose(filedescriptor, fileCallback, (void*)&cd);
  while (cd > 0); //wait until data has been altered
  // Change thread state to Stop waiting.
  if (cd < 0)
  {
    return VM_STATUS_FAILURE;
  }//negative result is a failure
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileClose(int filedescriptor)


TVMStatus VMFileWrite(int filedescriptor, void *data, int *length)
{
  cd = -739;
  if (!data || !length)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
  MachineFileWrite(filedescriptor, data, *length, fileCallback, (void*)&cd);
  while (cd == -739);
  if(cd >= 0)
  {
    return VM_STATUS_SUCCESS;
  }
  return VM_STATUS_FAILURE;
} // VMFileWrite


TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset)
{
  // void MachineFileSeek(int fd, int offset, int whence, TMachineFileCallback callback, void *calldata);
  cd = -728;
  // Make thread wait
  MachineFileSeek(filedescriptor, offset, whence, fileCallback, (void*)&cd);
  while (cd == -728);
  if (newoffset)
  {
    *newoffset = cd;
  }
  //make thread stop waiting
  if (cd < 0)
    return VM_STATUS_FAILURE;
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMFileRead(int filedescriptor, void *data, int *length)
{
  cd = -728;

  if (!data || !length)
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  // Make thread wait
  MachineFileRead(filedescriptor, data, *length, fileCallback, (void*)&cd);
  while (cd == -728);
  *length = cd;
  //make thread stop waiting

  if (cd < 0)
    return VM_STATUS_FAILURE;
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMThreadSleep(TVMTick tick)
{
  ticks = 0; //set this.thread->resetTicks()
  if (tick == VM_TIMEOUT_IMMEDIATE)
  {
    // the current process yields the remainder of its processing quantum to the next ready process of equal priority.
  }
  else if (tick == VM_TIMEOUT_INFINITE)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
  else
  {
    while (ticks < tick);
    //this should not just be a blind wait loop; should probably pass run time to next thread. Update later.
  } //does nothing while the number of ticks that have passed since entering this function is less than the number to sleep on

  return VM_STATUS_SUCCESS;
}


TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)
{
  Thread* t = new Thread;
  t->setPriority(prio);
  t->setID(*tid);
  if (!entry || !tid)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//INVALID PARAMS, must not be NULL
  (void*) stackaddr = new (void*)(memsize);
  threads->push_back(t);
//  void MachineContextCreate(SMachineContextRef mcntxref, void (*entry)(void *), void *param, void *stackaddr, size_t stacksize);
  MachineContextCreate(t->getContextRef(), (void*)entry, param, (void*) stackaddr, size_t memsize);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)


void fileCallback(void* calldata, int result)
{
//MachineFile* callback function
  *(int*)calldata = result;
}//void fileCallback(void* calldata, int result)


void incrementTicks(void*)
{
  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    itr->incrementTicks();
  }//add one tick passed to every thread
}//helper function for sleeping



