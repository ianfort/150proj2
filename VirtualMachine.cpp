#include "Thread.h"


using namespace std;

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);
void fileCallback(void* calldata, int result);
void timerISR(void*);
void idle(void*);
void scheduler();

TVMThreadID nextID; //increment every time a thread is created. Decrement never.
vector<Thread*> *threads;
Thread *tr, *mainThread, *pt, *sched;
queue<Thread*> *readyQ[NUM_RQS];
sigset_t sigs;

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
  sched = new Thread;
  mainThread = new Thread;
  mainThread->setPriority(VM_THREAD_PRIORITY_NORMAL);
  mainThread->setState(VM_THREAD_STATE_RUNNING);
  mainThread->setID(nextID);
  nextID++;
  threads->push_back(mainThread);
  tr = mainThread;
  VMThreadCreate(idle, NULL, 0x100000, VM_THREAD_PRIORITY_NIL, &idletid);
  VMThreadActivate(idletid);
  MachineInitialize(machinetickms);
  MachineRequestAlarm((tickms*1000), timerISR, NULL);
  MachineEnableSignals();

  mainFunc(argc, argv);
  VMUnloadModule();
  MachineTerminate();
  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    if ((*itr))
    {
      delete *itr;
    }//delete contents of threads
  }//for all threads
  delete threads;
  return VM_STATUS_SUCCESS;
} //VMStart


TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor)
{
  tr->setcd(-18); //impossible to have a negative file descriptor
  if (filename == NULL || filedescriptor == NULL)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//need to have a filename and a place to put the FD
  MachineFileOpen(filename, flags, mode, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if(tr->getcd() < 0)
  {
    return VM_STATUS_FAILURE;
  }//if invalid FD
  *filedescriptor = tr->getcd();
  return VM_STATUS_SUCCESS;
} // VMFileOpen


TVMStatus VMFileClose(int filedescriptor)
{
  tr->setcd(1);
  // Make thread wait here.
  MachineFileClose(filedescriptor, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if (tr->getcd() < 0)
  {
    return VM_STATUS_FAILURE;
  }//negative result is a failure
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileClose(int filedescriptor)


TVMStatus VMFileWrite(int filedescriptor, void *data, int *length)
{
  tr->setcd(-739);
  if (!data || !length)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//not allowed to have NULL pointers for where we put the data
  MachineFileWrite(filedescriptor, data, *length, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if(tr->getcd() < 0)
  {
    return VM_STATUS_FAILURE;
  }//if a non-negative number of bytes written
  return VM_STATUS_SUCCESS;
} // VMFileWrite


TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset)
{
  tr->setcd(-728);
  MachineFileSeek(filedescriptor, offset, whence, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if (newoffset)
  {
    *newoffset = tr->getcd();
  }
  if (tr->getcd() < 0)
    return VM_STATUS_FAILURE;
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMFileRead(int filedescriptor, void *data, int *length)
{
  tr->setcd(-728);
  if (!data || !length)
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  MachineFileRead(filedescriptor, data, *length, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  *length = tr->getcd();
  if (tr->getcd() < 0)
    return VM_STATUS_FAILURE;
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMThreadSleep(TVMTick tick)
{
  if (tick == VM_TIMEOUT_IMMEDIATE)
  {
    tr->setState(VM_THREAD_STATE_READY);
    readyQ[tr->getPriority()]->push(tr);
    scheduler();
  }// the current process yields the remainder of its processing quantum to the next ready process of equal priority.
  else if (tick == VM_TIMEOUT_INFINITE)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//invalid param
  else
  {
    tr->setTicks(tick);
    tr->setState(VM_THREAD_STATE_WAITING);
    scheduler();
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
//  MachineResumeSignals(&sigs);
  MachineSuspendSignals(&sigs);
  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    (*itr)->decrementTicks();
  }//add one tick passed to every thread
  MachineResumeSignals(&sigs);
  scheduler();
}//Timer ISR: Do Everything!


void scheduler()
{
  pt = tr;
  if (pt->getState() == VM_THREAD_STATE_RUNNING)
  {
    pt->setState(VM_THREAD_STATE_READY);
    readyQ[pt->getPriority()]->push(pt);
  }//return non-blocked old thread to ready queues
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
    readyQ[VM_THREAD_PRIORITY_NIL]->pop();
  }//if there's nothing in any of the RQs, spin with the idle process
  tr->setState(VM_THREAD_STATE_RUNNING);
  MachineContextSwitch(pt->getContextRef(), tr->getContextRef());
}//void scheduler()


void idle(void*)
{
  while(1);
}//idles forever!


