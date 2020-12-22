
#include <iostream>

#include <caen_lib.h>

#include <daq_device_caentdc.h>
#include <string.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#define VME_INTERRUPT_LEVEL      0
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK

#define MAX_MBLT_SIZE            (256*1024)

using namespace std;

daq_device_caentdc::daq_device_caentdc(const int eventtype
                                       , const int subeventid
                                       , const int linknumber
                                       , const int boardnumber
                                       , const int address
                                       , const int trigger)
{
  m_eventType  = eventtype;
  m_subeventid = subeventid;

  _linknumber = linknumber;
  _boardnumber = boardnumber;
  tdc_mAddr = address;

  tdc_Handle = 0;
  _warning = 0;
  _broken = 0;

  //! Initiate the VME bridge and perform ADC reset
  cout << "*************** opening VME Bridge" << endl;
  tdc_connect((short)_linknumber,(short)_boardnumber);
  if ( tdc_isConnected )
    {
      cout << "Connected to VME Bridge! Reseting TDC" << tdc_isConnected << endl;
    }else{
      cout<<"Errors!"<<endl;
      exit(0);
  }
  // TDC init (software reset the board)
  tdc_init();

  //! no effect for now, reserved for future developements
  //! trigger handler
  _trigger_handler=0;
  if (trigger)   _trigger_handler=1;

  if ( _trigger_handler )
    {
      cout << __LINE__ << "  " << __FILE__ << " registering triggerhandler " << endl;
      _th  = new CAENdrsTriggerHandler(tdc_Handle, m_eventType);
      registerTriggerHandler ( _th);
    }
  else
    {
      _th = 0;
    }
}

daq_device_caentdc::~daq_device_caentdc()
{

  if (_th)
    {
      clearTriggerHandler();
      delete _th;
      _th = 0;
    }

}


//!--------------------------tdc lib----------------------------

void daq_device_caentdc::tdc_connect(short linkNum, short boardNum){
    int ret=0;
    tdc_Handle=init_nbbqvio(&ret,linkNum,boardNum);
    tdc_isConnected=ret;
}
void daq_device_caentdc::tdc_init()
{
    // software reset the board (single shot)
  short sval;
  sval = 0x0;
  vwrite16(tdc_Handle,tdc_mAddr + V775_SOFT_CLEAR, &sval);
  // perform setup
  //system("./v1190.sh");//external command here
}

int daq_device_caentdc::tdc_blockread_segdata(char* blkdata,int cnt)
{
    int sz, rsz;
    sz = cnt *4;
    rsz = dma_vread32(tdc_Handle,tdc_mAddr,blkdata, sz);
    return rsz;//return size
}
int daq_device_caentdc::tdc_read_segdata(int *data)
{
    vread32(tdc_Handle,tdc_mAddr + V1190_OUTBUFF, data);
    return 1;
}
void daq_device_caentdc::tdc_clear(){
    //in case fifo is not used
    short sval;
    sval = 0x04;
    vwrite16(tdc_Handle,tdc_mAddr + V775_BIT_SET2, &sval);
    vwrite16(tdc_Handle,tdc_mAddr + V775_BIT_CLE2, &sval);
}
void daq_device_caentdc::tdc_clearEvtCounter(){
  short sval;
  sval = 0x00;
  vwrite16(tdc_Handle,tdc_mAddr + V775_BIT_CLEEVTCNT, &sval);
}
void daq_device_caentdc::tdc_noberr(){
  short sval = 0x00;

  vwrite16(tdc_Handle,tdc_mAddr + V775_CTRL_REG1, &sval);
}

void daq_device_caentdc::tdc_multievtberr(){
  short sval = 0x20;

  vwrite16(tdc_Handle,tdc_mAddr + V775_CTRL_REG1, &sval);
}
void daq_device_caentdc::tdc_intlevelmulti(short level, short evtn){
    vwrite16(tdc_Handle,tdc_mAddr + V775_EVT_TRIG_REG, &evtn);
    vwrite16(tdc_Handle,tdc_mAddr + V775_INT_REG1, &level);
}
void daq_device_caentdc::tdc_intlevel(short level){
    this->tdc_intlevelmulti(level, 1);
}
//!--------------------------tdc lib----------------------------

int  daq_device_caentdc::init()
{

  if ( _broken )
    {
      return 0; //  we had a catastrophic failure
    }
  receivedTrigger = 0;
  blockCounter = 0;
  tdc_clearEvtCounter();
  tdc_clear();
  tdc_noberr();
  tdc_multievtberr();
  tdc_intlevel(VME_INTERRUPT_LEVEL);

  rearm (m_eventType);
  return 0;

}

// the put_data function

int daq_device_caentdc::put_data(const int etype, int * adr, const int length )
{

  if ( _broken )
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }

  if ( length < max_length(etype) )
    {
      //      cout << __LINE__ << "  " << __FILE__ << " length " << length <<endl;
      return 0;
    }


  int len = 0;

  sevt =  (subevtdata_ptr) adr;
  // set the initial subevent length
  sevt->sub_length =  SEVTHEADERLENGTH;

  // update id's etc
  sevt->sub_id =  m_subeventid;
  sevt->sub_type=4;
  sevt->sub_decoding = 85;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;

  uint32_t buffersize = 0;

  //! tdc reading from here
  int  *d;
  int ipos=0;
  d=(int*) &sevt->data;

  //! header
  *d++ = blockCounter;//previous recorded data block
  ipos++;
  *d++ = receivedTrigger;//previous recorded trigger
  ipos++;
  *d++ = 0;//(int)lupo->readTimeStampMSB32();
  ipos++;
  *d++ = 0;//(int)lupo->readTimeStampLSB32();
  ipos++;

  //! data
  //! identifier bit for QDC (used in case of ADC-TDC sequential readout)
  *d++ = V775_SEPARATION_BIT;
  ipos++;

  //! read tdc
  uint32_t tdcbuffersize=tdc_blockread_segdata((char*)(d++),MAX_MBLT_SIZE);
  buffersize+=tdcbuffersize;
  d+=tdcbuffersize/4;

  //! end of tdc bit
  *d++ = V775_SEPARATION_BIT;
  ipos+=2;
  receivedTrigger+=tdcbuffersize/72;// total number of events
  *d++ = receivedTrigger;
  ipos++;

  if (tdcbuffersize>0) blockCounter++;

  //! calculations of event lenght
  len = buffersize /4;
  sevt->sub_padding = ipos+len;
  len += len%2;
  // this is to discard empty event
  if (tdcbuffersize>0){
      sevt->sub_length += ipos+len;
      blockCounter++;
  }else{
      sevt->sub_length += 0;
  }
  //cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;
  return  sevt->sub_length;
}


int daq_device_caentdc::endrun()
{
  if ( _broken )
    {
      return 0; //  we had a catastrophic failure
    }
  //  cout << __LINE__ << "  " << __FILE__ << " ending run " << endl;
}


void daq_device_caentdc::identify(std::ostream& os) const
{

}


int daq_device_caentdc::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (14900);
}


// the rearm() function
int  daq_device_caentdc::rearm(const int etype)
{
  if ( _broken )
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  if (etype != m_eventType) return 0;

  return 0;
}



