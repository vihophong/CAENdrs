#include "CAENdrs_plugin.h"

#include "parseargument.h"

#include <daq_device_caenadc.h>
#include <daq_device_caentdc.h>
#include <daq_device_caen_v1730.h>


#include <sstream>
#include <string>
#include <strings.h>

int get_value_hex(const char *arg){
    std::istringstream sarg ( arg);
    int value;
    std::string s = arg;

    int x =  s.find("0x",0);


    if (x == -1 )
      {
        sarg >> value;
      }
    else
      {
        sarg >> std::hex >> value;
      }

    return value;
}

unsigned int get_uvalue_hex(const char *arg){
    std::istringstream sarg ( arg);
    unsigned int value;
    std::string s = arg;

    int x =  s.find("0x",0);


    if (x == -1 )
      {
        sarg >> value;
      }
    else
      {
        sarg >> std::hex >> value;
      }

    return value;
}

int CAENdrs_plugin::create_device(deviceblock *db)
{

  std::cout << __LINE__ << "  " << __FILE__ << "  " << db->npar << "  " 
	    << db->argv0 << "  " 
	    << db->argv1 << "  " 
	    << db->argv2 << "  " 
	    << db->argv3 << "  " 
	    << db->argv4 << "  " 
	    << db->argv5 << "  " 
	    << db->argv6 << "  " 
	    << std::endl;

  int eventtype;
  int subid;


  if ( strcasecmp(db->argv0,"device_caenadc") == 0 )
    {
      // we need at least 5 params
      if ( db->npar <6 ) return 1; // indicate wrong params
      
      eventtype  = get_value ( db->argv1); // event type
      subid = get_value ( db->argv2); // subevent id
      int linknumber = get_value ( db->argv3);
      int boardnumber = get_value ( db->argv4);
      int address = get_value_hex ( db->argv5);
      if ( db->npar == 6)
        {
          add_readoutdevice ( new daq_device_caenadc( eventtype,
                                                          subid,
                                                          linknumber,
                                                          boardnumber,
                                                          address ));
          return 0;  // say "we handled this request"
        }
      else if ( db->npar == 7)
        {
          int trigger = get_value ( db->argv6);
          add_readoutdevice ( new daq_device_caenadc( eventtype,
                                                          subid,
                                                          linknumber,
                                                          boardnumber,
                                                          address,
                                                          trigger ));
          return 0;  // say "we handled this request"
        }
    }
  
  else if ( strcasecmp(db->argv0,"device_caentdc") == 0 )
    {
      // we need at least 5 params
      if ( db->npar <6 ) return 1; // indicate wrong params

      eventtype  = get_value ( db->argv1); // event type
      subid = get_value ( db->argv2); // subevent id
      int linknumber = get_value ( db->argv3);
      int boardnumber = get_value ( db->argv4);
      int address = get_value_hex ( db->argv5);
      if ( db->npar == 6)
        {
          add_readoutdevice ( new daq_device_caentdc( eventtype,
                                                          subid,
                                                          linknumber,
                                                          boardnumber,
                                                          address ));
          return 0;  // say "we handled this request"
        }
      else if ( db->npar == 7)
        {
          int trigger = get_value ( db->argv6);
          add_readoutdevice ( new daq_device_caentdc( eventtype,
                                                          subid,
                                                          linknumber,
                                                          boardnumber,
                                                          address,
                                                          trigger ));
          return 0;  // say "we handled this request"
        }
    }
  else if ( strcasecmp(db->argv0,"device_caen_v1730") == 0 )
    {
      // we need at least 5 params
      if ( db->npar <5 ) return 1; // indicate wrong params

      eventtype  = get_value ( db->argv1); // event type
      subid = get_value ( db->argv2); // subevent id
      int isdppmode = get_value ( db->argv3);
      int boardid = get_value ( db->argv4);

      if ( db->npar == 5)
        {
          add_readoutdevice ( new daq_device_caen_v1730( eventtype,
                                                          subid,
                                                          isdppmode,
                                                          boardid ));
          return 0;  // say "we handled this request"
      }else if ( db->npar == 6)
        {
          int trigger = get_value ( db->argv5);
          add_readoutdevice ( new daq_device_caen_v1730( eventtype,
                                                          subid,
                                                          isdppmode,
                                                          boardid,
                                                          trigger ));
          return 0;  // say "we handled this request"
        }

    }
  return -1; // say " this is not our device"
}

void CAENdrs_plugin::identify(std::ostream& os, const int flag) const
{

  if ( flag <=2 )
    {
      os << " - CAEN DRS Plugin" << std::endl;
    }
  else
    {
      os << " - CAEN DRS Plugin, provides - " << std::endl;
      os << " -     device_CAENdrs (evttype, subid, link_nr, trigger) - CAEN V1742 custom config" << std::endl;
      os << " -     device_CAENdrs_std (evttype, subid, link_nr, trigger, speed, delay[%]) - CAEN V1742 standard config" << std::endl;
    }
      

}

CAENdrs_plugin _cdp;
