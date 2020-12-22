
#include "CAENdrsTriggerHandler.h"
#include <iostream>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

using namespace std;


int CAENdrsTriggerHandler::wait_for_trigger( const int moreinfo)
{
  /*
  const int timeout = 500;
  int ret = CAEN_DGTZ_IRQWait(_handle, timeout);
  
  if (ret == CAEN_DGTZ_Timeout) return 0;
  
  if (ret != CAEN_DGTZ_Success)  
    {
      cout << __LINE__ << "  " << __FILE__ << " Error from IRQwait " << ret  << endl;
      return 0;
    }
  */
  //cout<<"trigger wait"<<endl;

  return _etype;
}
