#ifndef __CAENDRSTRIGGERHANDLER_H__
#define __CAENDRSTRIGGERHANDLER_H__


#include "TriggerHandler.h"

class DRSBoard;

class CAENdrsTriggerHandler : public TriggerHandler {

public:

  CAENdrsTriggerHandler(int handle, const int etype = 1)
    {
      _handle = handle;
      _etype = etype;
    }

  ~CAENdrsTriggerHandler() {};

  //  virtual void identify(std::ostream& os = std::cout) const = 0;

  // this is the virtual worker routine
  int wait_for_trigger( const int moreinfo=0);

 protected:
  
  int _handle;
  int _etype ;


};

#endif
