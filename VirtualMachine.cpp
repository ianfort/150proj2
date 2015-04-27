#include "VirtualMachine.h"
#include "Machine.h"
#include <unistd.h>
#include <iostream>
#include <vector>
#include <queue>
#include "Thread.h"

#define NUM_RQS                                 4
#define VM_THREAD_PRIORITY_NIL                  ((TVMThreadPriority)0x00)

using namespace std;

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);
void fileCallback(void* calldata, int result);
void timerISR(void*);
void idle(void*);

TVMThreadID nextID; //increment every time a thread is created. Decrement never.
vector<Thread*> *threads;
Thread *tr, *mainThread;
queue<Thread*> *readyQ[NUM_RQS];


TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[])
{
  nextID = 0;
  TVMThreadID idletid;

  TVMMainEntry mainFunc = VMLoadModule(argv[0]);
  if (!mainFunc)
  {
    return VM_STATUS_FAILURE;
  }//if mainFunc doesn't load, kill yourself

  threads = new vector<Thread*>;
  for (int i = 0; i < NUM_RQS; i++)
  {
    readyQ[i] = new queue<Thread*>;
  }//allocate memory for ready queues
  mainThread = new Thread;
  mainThread->setPriority(VM_THREAD_PRIORITY_NORMAL);
  mainThread->setState(VM_THREAD_STATE_READY);
  mainThread->setID(nextID);
  nextID++;
  readyQ[mainThread->getPriority()]->push(mainThread);
  tr = mainThread;
  VMThreadCreate(idle, NULL, 0x100000, VM_THREAD_PRIORITY_NIL, &idletid);
  MachineInitialize(machinetickms);
  MachineRequestAlarm((tickms*1000), timerISR, NULL);
  MachineEnableSignals();

  mainFunc(argc, argv);
  VMUnloadModule();
  MachineTerminate();
  delete threads;
  return VM_STATUS_SUCCESS;
} //VMStart


TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor)
{
  tr->cd = -2; //impossible to have a negative file descriptor
  if (filename == NULL || filedescriptor == NULL)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
  //change thread state to VM_THREAD_STATE_WAITING
  MachineFileOpen(filename, flags, mode, fileCallback, (void*)&(tr->cd));
  // void MachineFileOpen(const char *filename, int flags, int mode, TMachineFileCallback callback, void *calldata);
  while((tr->cd) < 0); //wait until file is actually open
  *filedescriptor = tr->cd;
  //change thread state to VM_THREAD_STATE_READY
  return VM_STATUS_SUCCESS;
//  return VM_STATUS_FAILURE;
} // VMFileOpen


TVMStatus VMFileClose(int filedescriptor)
{
  tr->cd = 1;
  // Make thread wait here.
  MachineFileClose(filedescriptor, fileCallback, (void*)&(tr->cd));
  while (tr->cd > 0); //wait until data has been altered
  // Change thread state to Stop waiting.
  if (tr->cd < 0)
  {
    return VM_STATUS_FAILURE;
  }//negative result is a failure
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileClose(int filedescriptor)


TVMStatus VMFileWrite(int filedescriptor, void *data, int *length)
{
  tr->cd = -739;
  if (!data || !length)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
  MachineFileWrite(filedescriptor, data, *length, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  if(tr->cd >= 0)
  {
    return VM_STATUS_SUCCESS;
  }
  return VM_STATUS_FAILURE;
} // VMFileWrite


TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset)
{
  // void MachineFileSeek(int fd, int offset, int whence, TMachineFileCallback callback, void *calldata);
  tr->cd = -728;
  // Make thread wait
  MachineFileSeek(filedescriptor, offset, whence, fileCallback, (void*)&(tr->cd));
  while (tr->cd == -728);
  if (newoffset)
  {
    *newoffset = tr->cd;
  }
  //make thread stop waiting
  if (tr->cd < 0)
    return VM_STATUS_FAILURE;
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMFileRead(int filedescriptor, void *data, int *length)
{
  tr->cd = -728;

  if (!data || !length)
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  // Make thread wait
  MachineFileRead(filedescriptor, data, *length, fileCallback, (void*)&(tr->cd));
  while (tr->cd == -728);
  *length = tr->cd;
  //make thread stop waiting

  if (tr->cd < 0)
    return VM_STATUS_FAILURE;
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMThreadSleep(TVMTick tick)
{
  if (tick == VM_TIMEOUT_IMMEDIATE)
  {
    tr->setState(VM_THREAD_STATE_READY);
    readyQ[tr->getPriority()]->push(tr);
  }// the current process yields the remainder of its processing quantum to the next ready process of equal priority.
  else if (tick == VM_TIMEOUT_INFINITE)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//invalid param
  else
  {
    tr->setTicks(tick);
    tr->setState(VM_THREAD_STATE_WAITING);
  } //does nothing while the number of ticks that have passed since entering this function is less than the number to sleep on
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadSleep(TVMTick tick)


TVMStatus VMThreadID(TVMThreadIDRef threadref)
{
  if (!threadref)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//if threadref is a null pointer
  threadref = tr->getIDRef();
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadID(TVMThreadIDRef threadref)


TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef state)
{
  bool found = false;
  if (!state)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//if stateref is a null pointer

  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    if (*((*itr)->getIDRef()) == thread)
    {
      *state = (*itr)->getState();
      found = true;
    }//found correct thread
  }//linear search through threads vector for the thread ID in question
  if (!found)
    return VM_STATUS_ERROR_INVALID_ID;
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef state)


TVMStatus VMThreadActivate(TVMThreadID thread)
{
  bool found = false;
  Thread* nt = NULL;
  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    if (*((*itr)->getIDRef()) == thread)
    {
      nt = *itr;
      found = true;
    }//found correct thread
  }//linear search through threads vector for the thread ID in question
  if (!found)
    return VM_STATUS_ERROR_INVALID_ID;
  if (nt->getState() != VM_THREAD_STATE_DEAD)
    return VM_STATUS_ERROR_INVALID_STATE;
  nt->setState(VM_THREAD_STATE_READY);
  nt->getEntry();
  readyQ[nt->getPriority()]->push(nt);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadActivate(TVMThreadID thread)


TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)
{
  SMachineContext context;
  if (!entry || !tid)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//INVALID PARAMS, must not be NULL
  uint8_t *mem =  new uint8_t[memsize];
  Thread* t = new Thread(prio, VM_THREAD_STATE_DEAD, *tid = nextID, mem, memsize, entry, param);
  nextID++;
  threads->push_back(t);
  MachineContextCreate(&context, entry, param, (void*) mem, memsize);
  t->setContext(context);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)


void fileCallback(void* calldata, int result)
{
  Thread* cbt = (Thread*)calldata;
  cbt->setcd(result);
  while (cbt->getState() != VM_THREAD_STATE_WAITING);
  cbt->setState(VM_THREAD_STATE_READY);
  readyQ[cbt->getPriority()]->push(cbt);
}//void fileCallback(void* calldata, int result)


void timerISR(void*)
{
//  Thread *pt;

  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    (*itr)->decrementTicks();
  }//add one tick passed to every thread

//  pt = tr;
  if (!readyQ[VM_THREAD_PRIORITY_HIGH]->empty())
  {
    tr = readyQ[VM_THREAD_PRIORITY_HIGH]->front();
    readyQ[VM_THREAD_PRIORITY_HIGH]->pop();
  }//if there's anything in hi-pri, run it
  else if (!readyQ[VM_THREAD_PRIORITY_NORMAL]->empty())
  {
    tr = readyQ[VM_THREAD_PRIORITY_NORMAL]->front();
    readyQ[VM_THREAD_PRIORITY_NORMAL]->pop();
  }//if there's anything in med-pri, run it
  else if (!readyQ[VM_THREAD_PRIORITY_LOW]->empty())
  {
    tr = readyQ[VM_THREAD_PRIORITY_LOW]->front();
    readyQ[VM_THREAD_PRIORITY_LOW]->pop();
  }//if there's anything in low-pri, run it
  else
  {
    tr = readyQ[VM_THREAD_PRIORITY_NIL]->front();
    //need to pre-load this Q with the idle process (which just sleeps forever)
  }//if there's nothing in any of the RQs, spin
    MachineContextRestore(tr->getContextRef());
//    MachineContextSwitch(pt->getContextRef(), tr->getContextRef());
//  }//run thread for one tick
}//Timer ISR: Do Everything!


void idle(void*)
{
  while(1);
}//idles forever!


