#include "VirtualMachine.h"
#include "Machine.h"
#include <unistd.h>

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);
void fileCallback(void* calldata, int result);
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
  *filedescriptor = -1; //impossible to have a negative file descriptor
  if (filename == NULL || filedescriptor == NULL)
  {
    return VM_STATUS_ERROR_INVALID_PARAMETER;
  }
  //change thread state to VM_THREAD_STATE_WAITING
  MachineFileOpen(filename, flags, mode, fileCallback, (void*)filedescriptor);
  // void MachineFileOpen(const char *filename, int flags, int mode, TMachineFileCallback callback, void *calldata);
  while(*filedescriptor < 0); //wait until file is actually open
  //change thread state to VM_THREAD_STATE_READY
  return VM_STATUS_SUCCESS;
//  return VM_STATUS_FAILURE;
} // VMFileOpen


TVMStatus VMFileClose(int filedescriptor)
{
  int data = 1;

  // Make thread wait here.
  MachineFileClose(filedescriptor, fileCallback, (void*)data);
  while (data > 0); //wait until data has been altered
  // Change thread state to Stop waiting.

  if ( data < 0 )
  {
    return VM_STATUS_FAILURE;
  }//negative result is a failure

  return VM_STATUS_SUCCESS;
}


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


TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset)
{
  // void MachineFileSeek(int fd, int offset, int whence, TMachineFileCallback callback, void *calldata);
  int no = -728;

  // Make thread wait
  MachineFileSeek(filedescriptor, offset, whence, fileCallback, (void*)&no);

  while (no == -728);
  if (newoffset)
  {
    no = *newoffset;
  }
  //make thread stop waiting

  return VM_STATUS_SUCCESS;
}//TVMStatus VMFileseek(int filedescriptor, int offset, int whence, int *newoffset)


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


void fileCallback(void* calldata, int result)
{
//MachineFileOpen callback function
  calldata = (void*)result;
}//void fileOpen(void* calldata, int result)


void incrementTicks(void*)
{
  ticks += 1;
}//helper function for sleeping



