#include "VirtualMachine.h"
#include "Machine.h"
#include <unistd.h>

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);
void incrementTicks(void*);

volatile unsigned int ticks;


TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[])
{
  ticks = 0;
  TVMMainEntry mainFunc = VMLoadModule(argv[0]);
  MachineInitialize(machinetickms);
  MachineRequestAlarm((tickms*1000), incrementTicks, NULL);
  MachineEnableSignals();
  if (mainFunc)
  {
    mainFunc(argc, argv);
    VMUnloadModule();
    return VM_STATUS_SUCCESS;
  }
  
  return VM_STATUS_FAILURE;
} //VMStart


TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor)
{
  if (filename == NULL || filedescriptor == NULL)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
  //change thread state to VM_THREAD_STATE_WAITING
  MachineFileOpen(filename, flags, mode, filedescriptor, NULL);
  // void MachineFileOpen(const char *filename, int flags, int mode, TMachineFileCallback callback, void *calldata);

  //change thread state to VM_THREAD_STATE_READY
  return VM_STATUS_SUCCESS;
//  return VM_STATUS_FAILURE;
} // VMFileOpen


TVMStatus VMFileWrite(int filedescriptor, void *data, int *length)
{
  if (!data || !length)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
//  MachineFileWrite(filedescriptor, data, length, , );
  if(write(filedescriptor, data, *length))
  {
    return VM_STATUS_SUCCESS;
  }
  return VM_STATUS_FAILURE;
} // VMFileWrite


TVMStatus VMThreadSleep(TVMTick tick)
{
  ticks = 0;
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




void incrementTicks(void*)
{
  ticks += 1;
}//helper function for sleeping



