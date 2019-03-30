
#include <iostream>

#include <sstream>
#include <parseargument.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "getopt.h"


//#include <CAENDigitizer.h>
//#include <CAENDigitizerType.h>

#include "caen_manager.h"

using namespace std;

int show_status( caen_manager *);


void exithelp( const int ret =1)
{

  cout  << std::endl;
  cout << " caen_client usage " << std::endl;
  cout << "    -d link (optical fiber) number,  default 0 " << std::endl;
  cout << "    -v increase verbosity, multiple -v for more" << std::endl;
  cout <<  std::endl;
  cout << "   caen_client status                    provide an overview of most registers" << std::endl;
  cout << "   caen_client identify                  identify the module" << std::endl;
  cout << "   caen_client init                      perform a standard initialization" << std::endl;

  cout << "   caen_client SetMaxNumEventsBLT        set number of events"   << endl;
  cout << "   caen_client SetExtTriggerInputMode    set the trigger input mode (0-3)"  << endl;
  cout << "   caen_client SetFastTriggerDigitizing  0/1"   << endl;
  cout << "   caen_client SetFastTriggerMode        0/1 "   << endl;
  cout << "   caen_client SetOutputSignalMode       (0 -7) "   << endl;
  cout << "   caen_client SetDRS4SamplingFrequency  (0 - 5Gs, 1 - 2.5GS, 2 - 1GS)"   << endl;
  cout << "   caen_client SetRecordLength           set the number of samples"   << endl;
  cout << "   caen_client SetPostTriggerSize        set the number of samples past a trigger"   << endl;

  cout << "   caen_client SetIOLevel                0 - NIM, 1 - TTL" << endl; 
  cout << "   caen_client SetAcquisitionMode        0 - software, 1 - interrupt" << endl;
  cout << "   caen_client SetGroupEnableMask        0xff for all" << endl;
  cout << "   caen_client LoadDRS4CorrectionData    for 0 - 5Gs, 1 - 2.5GS, 2 - 1GS"   << endl;
  cout << "   caen_client EnableDRS4Correction      enable the loaded correction data" << endl;

  cout << "   caen_client SetTriggerPolarity channel value       set the trigger polarity for this channel (0/1)"   << endl;
  cout << "   caen_client SetTriggerPolarity value               set the trigger polarity for all channels (0/1)"   << endl;

  cout << "   caen_client SetChannelDCOffset channel value       set DAC offset for channel" << std::endl;
  cout << "   caen_client SetChannelDCOffset value               set DAC offset for all channels" << std::endl;

  cout << "   caen_client WriteRegister address value            low-level write to a register" << std::endl;
  cout << "   caen_client ReadRegister address                   low-level read from a register" << std::endl;


  //  cout << "   caen_client reset     full board reset" << std::endl;

  cout << "    " << std::endl;
  exit(ret);
}

main(int argc, char *argv[])
{

  if ( argc==1) 
    {
      exithelp(1);
    }

  int verbosity = 0;
  int linknum = 0;

  int opt;
  while ((opt = getopt(argc, argv, "hvd:")) != EOF)
    {
      switch (opt) 
	{
	  // the -d  (which linknum) switch 
	case 'd':
	  if ( !sscanf(optarg, "%d",  &linknum) ) exithelp();
	  if ( linknum < 0 || linknum > 3) exithelp();
	  break;

	case 'v':
	  verbosity++;
	  break;

	case 'h':
	  exithelp(0);
	  break;

	default:
	  break;
						
	}
    }


  caen_manager * cm = new caen_manager(linknum);
  if (cm->GetStatus())
    {
      cout << "Failure, Status = "  << cm->GetStatus() << endl;
      delete cm;
      return 1;
    }

  unsigned short reg;
  unsigned int val = 0;
  unsigned int index = 0;
  int i;
  int status=0;

  if ( strcmp( argv[optind], "status") == 0)  
    {
      cout << " Status"  << endl;
      show_status(cm);
      delete cm;
      return 0;
    }
  
  if ( argc > optind + 1 )  //we have 2+ parameters, say "level 16000" 
    {

      if ( strcmp( argv[optind], "SetChannelDCOffset") == 0)  
	{
	  if ( argc > optind + 2 )  // "SetChannelDCOffset 10 16000"
	    {
	      index = get_uvalue( argv[optind+1]);
	      val   = get_uvalue( argv[optind+2]);
	      //cout << " setting level " << index << "  " << val << endl;
	      status = cm->SetChannelDCOffset(index, val);
	    }
	  else
	    {
	      val   = get_uvalue( argv[optind+1]);
	      //cout << " setting all levels to  " << val << endl;
	      status = cm->SetChannelDCOffset(val);
	    }
	}

      else if ( strcmp( argv[optind], "SetMaxNumEventsBLT") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  status = cm->SetMaxNumEventsBLT(val);

	  cout << __FILE__ << " " << __LINE__ << " value  " << cm->ReadRegister(0xEF1C)  << endl;
	  
	}
  
      else if ( strcmp( argv[optind], "SetExtTriggerInputMode") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetExtTriggerInputMode(val);
	}
      
      else if ( strcmp( argv[optind], "SetTriggerPolarity") == 0)  
	{
	  if ( argc > optind + 2 ) 
	    {
	      index = get_uvalue( argv[optind+1]);
	      val   = get_uvalue( argv[optind+2]);
	      status = cm->SetTriggerPolarity(index, val);
	    }
	  else
	    {
	      val   = get_uvalue( argv[optind+1]);
	      //  cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	      status = cm->SetTriggerPolarity(val);
	    }
	}
  
      else if ( strcmp( argv[optind], "SetFastTriggerDigitizing") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetFastTriggerDigitizing(val);
	}

      else if ( strcmp( argv[optind], "SetFastTriggerMode") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetFastTriggerMode(val);
	}

      else if ( strcmp( argv[optind], "SetOutputSignalMode") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetOutputSignalMode(val);
	}

      else if ( strcmp( argv[optind], "SetDRS4SamplingFrequency") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetDRS4SamplingFrequency(val);
	}

      else if ( strcmp( argv[optind], "SetRecordLength") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetRecordLength(val);
	}
      
      else if ( strcmp( argv[optind], "SetPostTriggerSize") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetPostTriggerSize(val);
	}
      
      else if ( strcmp( argv[optind], "SetIOLevel") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetIOLevel(val);
	}
      
      else if ( strcmp( argv[optind], "SetAcquisitionMode") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetAcquisitionMode(val);
	}

      else if ( strcmp( argv[optind], "SetGroupEnableMask") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->SetGroupEnableMask(val);
	}
      
      else if ( strcmp( argv[optind], "LoadDRS4CorrectionData") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->LoadDRS4CorrectionData(val);
	}
      
      else if ( strcmp( argv[optind], "EnableDRS4Correction") == 0)  
	{
	  val = get_uvalue( argv[optind+1]);
	  //cout << __FILE__ << " " << __LINE__ << " setting to  " << val << endl;
	  status = cm->EnableDRS4Correction(val);
	}

      else if ( strcmp( argv[optind], "WriteRegister") == 0)  
	{
	  if ( argc > optind + 2 ) 
	    {
	      index = get_uvalue( argv[optind+1]);
	      val   = get_uvalue( argv[optind+2]);
	      status = cm->WriteRegister(index, val);
	    }
	  else
	    {
	      status = -1;
	    }
	}
      
      else if ( strcmp( argv[optind], "ReadRegister") == 0)  
	{
	  index = get_uvalue( argv[optind+1]);
	  val = cm->ReadRegister(index);
	  cout  << "Address 0x" << hex << index << ":  0x" << val << dec  << endl;
	  status =0;
	  if ( val < 0) status = val;
	}

     
    }

  else
    {

      if ( strcmp( argv[optind], "init") == 0)  
	{
	  //cout << " init  " << val << endl;
	  status = cm->init();
	}

      else if ( strcmp( argv[optind], "identify") == 0)  
	{
	  cm->identify();
	}
      else if ( strcmp( argv[optind], "help") == 0)  
	{
	  delete cm;
	  exithelp(0);
	}


    }

  if (status)  cout << "Failure, Status = " << status << " cm status = " << cm->GetStatus() << endl;

  return 0;
}


const char *getTriggerMnemomic(const int i)
{ 

  switch (i)
    {
    case 0:
      return "DISABLED";
      break;


    case 1:
      return "EXTOUT_ONLY";
      break;


    case 2:
      return "ACQ_ONLY";
      break;

    case 3:
      return "ACQ_AND_EXTOUT";
      break;

    default:
      return "unknown";
    }
}

const char *getOutputSignalModeMnemonic(const int i)
{ 

  switch ( i) 
    {
    case 0:
      return "TRIGGER";
      break;

    case 1:
      return "FASTTRG_ALL";
      break;

    case 2:
      return "FASTTRG_ACCEPTED";
      break;
      
    case 3:
      return "BUSY";
      break;

    default:
      return "unknown";
    }
}

const char *GetDRS4SamplingFrequencyMnemonic(const int i)
{ 

  switch ( i) 
    {
    case 0:
      return "DRS4_5GHz";
      break;

    case 1:
      return "DRS4_2_5GHz";
      break;

    case 2:
      return "DRS4_1GHz";
      break;
      
     default:
      return "unknown";
    }
}




int show_status( caen_manager *cm)
{
  int i;

  cout << "MaxNumEventsBLT:       " << cm->GetMaxNumEventsBLT() << endl;
  cout << "ExtTriggerInputMode:   " << cm->GetExtTriggerInputMode() << " ("<< getTriggerMnemomic(cm->GetExtTriggerInputMode()) << ")" << endl;

  cout << "FastTriggerDigitizing: " << cm->GetFastTriggerDigitizing();
  if ( cm->GetFastTriggerDigitizing() ) cout << " (Enabled)";
  else cout << " (Disabled)";
  cout << endl;

  cout << "FastTriggerMode:       " << cm->GetFastTriggerMode();
  if ( cm->GetFastTriggerMode() ) cout << " (Enabled)";
  else cout << " (Disabled)";
  cout << endl;

  cout << "OutputSignalMode:      " << cm->GetOutputSignalMode() << " (" << getOutputSignalModeMnemonic(cm->GetOutputSignalMode()) << ")" << endl;
  cout << "Sampling Frequency:    " << cm->GetDRS4SamplingFrequency() << " (" << GetDRS4SamplingFrequencyMnemonic(cm->GetDRS4SamplingFrequency()) << ")" << endl;
  cout << "RecordLength:          " << cm->GetRecordLength() << " samples" << endl;
  cout << "PostTriggerSize:       " << cm->GetPostTriggerSize() << endl;

  cout << "IOLevel()              "  << cm->GetIOLevel();
  if ( cm->GetIOLevel()) cout << " (TTL)" << endl;
  else cout << " (NIM)" << endl;

  cout << "AcquisitionMode        "  << cm->GetAcquisitionMode();
  if ( cm->GetAcquisitionMode() ) cout << " (Interrrupt Controlled)" << endl;
  else cout << " (software controlled)" << endl;


  cout << "GroupEnableMask        0x"  <<  hex << cm->GetGroupEnableMask() << dec << endl;

  cout << "TriggerPolarity (F - falling edge / R - rising edge) :"<< endl;

  cout << "  ";
  for ( i = 0; i < 32; i++)
    {
      if ( cm->GetTriggerPolarity(i))   cout << "F  ";
      else cout << "R  ";
	   
      if ( (i+1)%8==0) cout << endl << "  ";
    }

  cout << endl << "ChannelDCOffset:"<< endl;
  cout << "  ";
  for ( i = 0; i < 32; i++)
    {
      cout << "0x" << hex << cm->GetChannelDCOffset(i) << dec << "  ";
      if ( (i+1)%8==0) cout << endl << "  ";
    }
  cout << endl;


  return 0;
}
