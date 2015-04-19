#include "VirtualMachine.h"
#include <unistd.h>

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);


TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[])
{
  TVMMainEntry mainFunc = VMLoadModule(argv[0]);
  if (mainFunc)
  {
    mainFunc(argc, argv);
    // TODO: Handle ticks
    VMUnloadModule();
    return VM_STATUS_SUCCESS;
  }
  
  return VM_STATUS_FAILURE;
} //VMStart


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

