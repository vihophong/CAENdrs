#include "CAENdrs_plugin.h"

#include "parseargument.h"

#include <daq_device_CAENdrs.h>
#include <daq_device_CAENdrs_std.h>

#include <strings.h>

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


  if ( strcasecmp(db->argv0,"device_CAENdrs") == 0 ) 
    {
      // we need at least 3 params
      if ( db->npar <3 ) return 1; // indicate wrong params
      
      eventtype  = get_value ( db->argv1); // event type
      subid = get_value ( db->argv2); // subevent id

      if ( db->npar == 3)
	{

	  add_readoutdevice ( new daq_device_CAENdrs( eventtype,
							  subid ));
	  return 0;  // say "we handled this request" 
	}

      else if ( db->npar == 4)
	{
	  int linknumber = get_value ( db->argv3);
	  
	  
	  add_readoutdevice ( new daq_device_CAENdrs( eventtype,
							  subid,
							  linknumber ));
	  return 0;  // say "we handled this request" 
	}
      
      else if ( db->npar == 5)
	{
	  int linknumber = get_value ( db->argv3);
	  int trigger = get_value ( db->argv4);

	  add_readoutdevice ( new daq_device_CAENdrs( eventtype,
							  subid,
							  linknumber, 
							  trigger ));
	  return 0;  // say "we handled this request" 
	}

    }
  
  else if ( strcasecmp(db->argv0,"device_CAENdrs_std") == 0 ) 
    {
      // we need at least 3 params
      if ( db->npar <3 ) return 1; // indicate wrong params
      
      eventtype  = get_value ( db->argv1); // event type
      subid = get_value ( db->argv2); // subevent id

      if ( db->npar == 3)
	{

	  add_readoutdevice ( new daq_device_CAENdrs_std( eventtype,
							  subid ));
	  return 0;  // say "we handled this request" 
	}

      else if ( db->npar == 4)
	{
	  int linknumber = get_value ( db->argv3);
	  
	  
	  add_readoutdevice ( new daq_device_CAENdrs_std( eventtype,
							  subid,
							  linknumber ));
	  return 0;  // say "we handled this request" 
	}
      
      else if ( db->npar == 5)
	{
	  int linknumber = get_value ( db->argv3);
	  int trigger = get_value ( db->argv4);

	  add_readoutdevice ( new daq_device_CAENdrs_std( eventtype,
							  subid,
							  linknumber, 
							  trigger ));
	  return 0;  // say "we handled this request" 
	}
      else if ( db->npar == 6)
	{
	  int linknumber = get_value ( db->argv3);
	  int trigger = get_value ( db->argv4);
	  int speed = get_value ( db->argv5);

	  add_readoutdevice ( new daq_device_CAENdrs_std( eventtype,
							  subid,
							  linknumber, 
							  trigger, 
							  speed ));
	  return 0;  // say "we handled this request" 
	}
      else if ( db->npar == 7)
	{
	  int linknumber = get_value ( db->argv3);
	  int trigger = get_value ( db->argv4);
	  int speed = get_value ( db->argv5);
	  int delay   = get_value ( db->argv6);

	  add_readoutdevice ( new daq_device_CAENdrs_std( eventtype,
							  subid,
							  linknumber, 
							  trigger, 
							  speed, 
							  delay ));
	  return 0;  // say "we handled this request" 
	}

      else
	{
	  return 1; // say it is our device but the parameters are wrong 
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
