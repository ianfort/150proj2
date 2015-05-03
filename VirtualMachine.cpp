#include "Thread.h"
#include "Tibia.h"
#include "Mutex.h"



using namespace std;

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);
void fileCallback(void* calldata, int result);
void timerISR(void*);
void scheduler();
void skeleton(void* tibia);
void idle(void*);

TVMThreadID nextID; //increment every time a thread is created. Decrement never.
vector<Thread*> *threads;
Thread *tr, *mainThread, *pt;
queue<Thread*> *readyQ[NUM_RQS];
sigset_t sigs;
vector<Mutex*> *mutexes;

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
  mutexes = new vector<Mutex*>;
  for (int i = 0; i < NUM_RQS; i++)
  {
    readyQ[i] = new queue<Thread*>;
  }//allocate memory for ready queues
  mainThread = new Thread;
  mainThread->setPriority(VM_THREAD_PRIORITY_NORMAL);
  mainThread->setState(VM_THREAD_STATE_RUNNING);
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
    if (*itr)
    {
      delete *itr;
    }//delete contents of threads
  }//for all threads
  
  for (vector<Mutex*>::iterator itr = mutexes->begin(); itr != mutexes->end(); itr++)
  {
    if (*itr)
    {
      delete *itr;
    }//delete contents of threads
  }//for all threads

  delete threads;
  delete mutexes;
  return VM_STATUS_SUCCESS;
} //VMStart


TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor)
{
  MachineSuspendSignals(&sigs);
  tr->setcd(-18); //impossible to have a negative file descriptor
  if (filename == NULL || filedescriptor == NULL)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//need to have a filename and a place to put the FD
  MachineFileOpen(filename, flags, mode, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if(tr->getcd() < 0)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_FAILURE;
  }//if invalid FD
  *filedescriptor = tr->getcd();
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
} // VMFileOpen


TVMStatus VMFileClose(int filedescriptor)
{
  MachineSuspendSignals(&sigs);
  tr->setcd(1);
  // Make thread wait here.
  MachineFileClose(filedescriptor, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if (tr->getcd() < 0)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_FAILURE;
  }//negative result is a failure
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileClose(int filedescriptor)


TVMStatus VMFileWrite(int filedescriptor, void *data, int *length)
{
  MachineSuspendSignals(&sigs);
  tr->setcd(-739);
  if (!data || !length)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//not allowed to have NULL pointers for where we put the data
  MachineFileWrite(filedescriptor, data, *length, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if(tr->getcd() < 0)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_FAILURE;
  }//if a non-negative number of bytes written
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
} // VMFileWrite


TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset)
{
  MachineSuspendSignals(&sigs);
  tr->setcd(-728);
  MachineFileSeek(filedescriptor, offset, whence, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  if (newoffset)
  {
    *newoffset = tr->getcd();
  }//return newoffset
  if (tr->getcd() < 0)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_FAILURE;
  }//check for failure
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMFileRead(int filedescriptor, void *data, int *length)
{
  MachineSuspendSignals(&sigs);
  tr->setcd(-728);
  if (!data || !length)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//not allowed to be NULL pointers
  MachineFileRead(filedescriptor, data, *length, fileCallback, (void*)tr);
  tr->setState(VM_THREAD_STATE_WAITING);
  scheduler();
  *length = tr->getcd();
  if (tr->getcd() < 0)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_FAILURE;
  }//if the calldata was invalid
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


TVMStatus VMThreadSleep(TVMTick tick)
{
  MachineSuspendSignals(&sigs);
  if (tick == VM_TIMEOUT_IMMEDIATE)
  {
    tr->setState(VM_THREAD_STATE_READY);
    readyQ[tr->getPriority()]->push(tr);
    scheduler();
  }// the current process yields the remainder of its processing quantum to the next ready process of equal priority.
  else if (tick == VM_TIMEOUT_INFINITE)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//invalid param
  else
  {
    tr->setTicks(tick);
    tr->setState(VM_THREAD_STATE_WAITING);
    scheduler();
  } //does nothing while the number of ticks that have passed since entering this function is less than the number to sleep on
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadSleep(TVMTick tick)


TVMStatus VMThreadID(TVMThreadIDRef threadref)
{
  MachineSuspendSignals(&sigs);
  if (!threadref)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//if threadref is a null pointer
  threadref = tr->getIDRef();
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadID(TVMThreadIDRef threadref)


TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef state)
{
  MachineSuspendSignals(&sigs);
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
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_ID;
  }//if thread wasn't a valid ID
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef state)


TVMStatus VMThreadActivate(TVMThreadID thread)
{
  MachineSuspendSignals(&sigs);
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
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_ID;
  }//if thread ID doesn't exist
  if (nt->getState() != VM_THREAD_STATE_DEAD)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_STATE;
  }//if thread is already dead
  nt->setState(VM_THREAD_STATE_READY);
  nt->getEntry();
  readyQ[nt->getPriority()]->push(nt);
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadActivate(TVMThreadID thread)


TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)
{
  MachineSuspendSignals(&sigs);
  SMachineContext context;
  if (!entry || !tid)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }//INVALID PARAMS, must not be NULL
  uint8_t *mem =  new uint8_t[memsize];
  Thread* t = new Thread(prio, VM_THREAD_STATE_DEAD, tid, mem, memsize, entry, param);
  threads->push_back(t);
  Tibia *tibia = new Tibia(entry, param);
  MachineContextCreate(&context, skeleton, (void*)tibia, (void*)mem, memsize);
  t->setContext(context);
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)


TVMStatus VMThreadDelete(TVMThreadID thread)
{
  MachineSuspendSignals(&sigs);
  Thread *del = NULL;
  vector<Thread*>::iterator killme;
  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    if (*((*itr)->getIDRef()) == thread)
    {
      del = *itr;
      killme = itr;
    }//save the correct itr for later use
  }//search through threads to find correct Thread* to delete
  if (!del)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_ID;
  }//thread ID not found
  if (del->getState() != VM_THREAD_STATE_DEAD)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_STATE;
  }//requested thread not dead
  threads->erase(killme);
  delete del;
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadDelete(TVMThreadID thread)


TVMStatus VMThreadTerminate(TVMThreadID thread)
{
  MachineSuspendSignals(&sigs);
  Thread *term = NULL, *temp;
  int target;
  for (vector<Thread*>::iterator itr = threads->begin(); itr != threads->end(); itr++)
  {
    if (*((*itr)->getIDRef()) == thread)
      term = *itr;
  }//search through threads to find correct Thread* to update
  if (!term)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_ID;
  }//term not found in threads
  if (term->getState() == VM_THREAD_STATE_DEAD)
  {
    MachineResumeSignals(&sigs);
    return VM_STATUS_ERROR_INVALID_STATE;
  }//requested thread already dead
  term->setState(VM_THREAD_STATE_DEAD);
  //release all of term's mutexes here
  target = readyQ[term->getPriority()]->size();
  for (int i = 0; i < target; i++)
  {
    temp = readyQ[term->getPriority()]->front();
    readyQ[term->getPriority()]->pop();
    if (temp != term)
      readyQ[term->getPriority()]->push(temp);
  }//cycle the relevant ready queue completely to find and flush the terminated thread from the ready queue
  scheduler();
  MachineResumeSignals(&sigs);
  return VM_STATUS_SUCCESS;
}//TVMStatus VMThreadTerminate(TVMThreadID thread)


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
  MachineResumeSignals(&sigs);
  MachineContextSwitch(pt->getContextRef(), tr->getContextRef());
}//void scheduler()


void skeleton(void *tibia)
{
  TVMThreadEntry func = ((Tibia*)tibia)->getEntry();
  func(((Tibia*)tibia)->getParam());
  VMThreadTerminate(*(tr->getIDRef()));
}//3spoopy5me


void idle(void*)
{
  while(1);
}//idles forever!


