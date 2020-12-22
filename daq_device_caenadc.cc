
#include <iostream>
#include <fstream>

#include <caen_lib.h>

#include <daq_device_caenadc.h>
#include <string.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>
#include <stdio.h>
#include <time.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <cstdlib>


#define VME_INTERRUPT_LEVEL      0
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK

#define MAX_MBLT_SIZE            (256*1024)

#define V785_EVT_SIZE            144;

using namespace std;

static long get_time()
{
    long time_ms;
#ifdef WIN32
    struct _timeb timebuffer;
    _ftime( &timebuffer );
    time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
#else
    struct timeval t1;
    struct timezone tz;
    gettimeofday(&t1, &tz);
    time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
#endif
    return time_ms;
}

int timeval_subtract (struct timespec *result, struct timespec* x, struct timespec*y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_nsec < y->tv_nsec) {
    int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
    y->tv_nsec -= 1000000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_nsec - y->tv_nsec > 1000000000) {
    int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
    y->tv_nsec += 1000000000 * nsec;
    y->tv_sec -= nsec;
  }
  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_nsec = x->tv_nsec - y->tv_nsec;
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

daq_device_caenadc::daq_device_caenadc(const int eventtype
                                       , const int subeventid
                                       , const int linknumber
                                       , const int boardnumber
                                       , const int address
                                       , const int trigger)
{
    const char* evn_fMAX_NEVT_FIFO_SEND=std::getenv("MAX_NEVT_FIFO_SEND");
    if (evn_fMAX_NEVT_FIFO_SEND)
        fMAX_NEVT_FIFO_SEND = atoi(evn_fMAX_NEVT_FIFO_SEND);
    else fMAX_NEVT_FIFO_SEND = 0;

    if (fMAX_NEVT_FIFO_SEND!=0){\
	cout<<"Attached with FIFO comm, MAX_NEVT_FIFO_SEND = "<<fMAX_NEVT_FIFO_SEND<<endl;
        //! fifo for interprocess communication
        sprintf(myfifo,"/tmp/myfifo");
        mkfifo(myfifo,0666);
        fifobuf=0;
        fifocal_prevEventCounter=0;
    }
    m_eventType  = eventtype;
    m_subeventid = subeventid;


    _linknumber = linknumber;
    _boardnumber = boardnumber;
    adc_mAddr = address<<16;

    adc_Handle = 0;
    _warning = 0;
    _broken = 0;

    //! Initiate the VME bridge and perform ADC reset
    cout << "*************** opening VME Bridge" << endl;
    if (_linknumber>=1000){
        std::ifstream handlefilein("handle");
        handlefilein>>adc_Handle;
        cout<<"Assigned Handle from previous device!"<<endl;
    }else{
        adc_connect((short)_linknumber,(short)_boardnumber);
        if ( adc_isConnected )
          {
            cout << "Connected to VME Bridge! Reseting ADC, status:" << adc_isConnected << endl;
            cout << "ADC based address = 0x" <<std::hex<<(adc_mAddr>>16)<<std::dec<<endl;
          }else{
            cout<<"\n\n Error opening the device\n"<<endl;
            exit(0);
        }
    }

    // ADC soft reset
    adc_init();

    _trigger_handler=0;
    if (trigger)   _trigger_handler=1;

    if ( _trigger_handler )
      {
        cout << __LINE__ << "  " << __FILE__ << " registering triggerhandler " << endl;
        _th  = new CAENdrsTriggerHandler (adc_Handle, m_eventType);
        registerTriggerHandler ( _th);
      }
    else
      {
        _th = 0;
      }
}



daq_device_caenadc::~daq_device_caenadc()
{
    if (_th)
    {
        clearTriggerHandler();
        delete _th;
        _th = 0;
    }
}



//!--------------------------adc lib----------------------------

void daq_device_caenadc::adc_connect(short linkNum, short boardNum){
    int ret=0;
    adc_Handle=init_nbbqvio(&ret,linkNum,boardNum);
    adc_isConnected=ret;
}
void daq_device_caenadc::adc_init()
{
  // reset the board
  short sval;
  sval = 0x0;
  vwrite16(adc_Handle,adc_mAddr + V792_SOFT_CLEAR, &sval);
  //system("./v792.sh");//external command here
}

int daq_device_caenadc::adc_blockread_segdata(char* blkdata,int cnt)
{
    int sz, rsz;
    //sz = cnt *4;
    sz = cnt;
    rsz = dma_vread32(adc_Handle,adc_mAddr,blkdata, sz);
    return rsz;//return size
}
int daq_device_caenadc::adc_read_segdata(int *data)
{
    vread32(adc_Handle,adc_mAddr + V792_OUTBUFF, data);
    return 1;
}
void daq_device_caenadc::adc_clear(){
  short sval;
  sval = 0x04;

  vwrite16(adc_Handle,adc_mAddr + V792_BIT_SET2, &sval);
  vwrite16(adc_Handle,adc_mAddr + V792_BIT_CLE2, &sval);
}
void daq_device_caenadc::adc_clearEvtCounter(){
  short sval;
  sval = 0x00;

  vwrite16(adc_Handle,adc_mAddr + V792_BIT_CLEEVTCNT, &sval);
}
void daq_device_caenadc::adc_noberr(){
  short sval = 0x00;

  vwrite16(adc_Handle,adc_mAddr + V792_CTRL_REG1, &sval);
}
void daq_device_caenadc::adc_berr(){
  short sval = 0x60;

  vwrite16(adc_Handle,adc_mAddr + V792_CTRL_REG1, &sval);
}

void daq_device_caenadc::adc_disable_suppression(){
  short sval = 0x0010;
  vwrite16(adc_Handle,adc_mAddr + 0x1032, &sval);
  sval = 0x0008;
  vwrite16(adc_Handle,adc_mAddr + 0x1032, &sval);
  sval = 0x1000;
  vwrite16(adc_Handle,adc_mAddr + 0x1032, &sval);
}

void daq_device_caenadc::adc_set_i_pedestal(short val){
    vwrite16(adc_Handle,adc_mAddr + 0x1060, &val);
}

void daq_device_caenadc::adc_multievtberr(){
  short sval = 0x20;

  vwrite16(adc_Handle,adc_mAddr + V792_CTRL_REG1, &sval);
}
void daq_device_caenadc::adc_intlevelmulti(short level, short evtn){
    vwrite16(adc_Handle,adc_mAddr + V792_EVT_TRIG_REG, &evtn);
    vwrite16(adc_Handle,adc_mAddr + V792_INT_REG1, &level);
}
void daq_device_caenadc::adc_intlevel(short level){
    this->adc_intlevelmulti(level, 1);
}

void daq_device_caenadc::adc_setthreshold(short ch, short threshold){
    unsigned int baddr=0x1080 + ch*2;
    short sval=threshold/16;
    vwrite16(adc_Handle,adc_mAddr +baddr,&sval);
}
//!--------------------------adc lib----------------------------



int  daq_device_caenadc::init()
{
    receivedTrigger = 0;
    blockCounter = 0;
    adc_intlevel(VME_INTERRUPT_LEVEL);

    adc_set_i_pedestal(100);
    //adc_noberr();
    //adc_multievtberr();
    adc_berr();
    // no threshold
    adc_disable_suppression();
    //with threshold
    //for (int i=0;i<32;i++) adc_setthreshold(i,500);

    adc_clearEvtCounter();
    adc_clear();


    int s = clock_gettime(CLOCK_MONOTONIC_RAW, &clk_begin);
    //rearm (m_eventType);
    return 0;
}

// the put_data function

int daq_device_caenadc::put_data(const int etype, int * adr, const int length )
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
    *d++ = receivedTrigger;//previous recievedTrigger
    ipos++;
    //! get time
    int s = clock_gettime(CLOCK_MONOTONIC_RAW, &clk);

    if  (s==0) {
        timeval_subtract(&clk_inteval,&clk,&clk_begin);
        *d++ = (int)clk_inteval.tv_sec;//previous recorded data block
        ipos++;
        //tv_nsec can not excceed 1000000000 so it is ok for casting
        *d++ = (int)clk_inteval.tv_nsec;
        ipos++;
    }else{
        *d++ = 0;//previous recorded data block
        ipos++;
        *d++ = 0;//previous recorded data block
        ipos++;
    }

    //! read qdc
    uint32_t qdcbuffersize=adc_blockread_segdata((char*)(d++),MAX_MBLT_SIZE);
    // this is to discard empty event
    if (qdcbuffersize==0) return 0;


    buffersize+=qdcbuffersize;
    d+=qdcbuffersize/sizeof(int);

    //! end of qdc bit
    *d++ = V792_SEPARATION_BIT;
    ipos+=2;
    receivedTrigger+=qdcbuffersize/V785_EVT_SIZE;// total number of events
    *d++ = receivedTrigger;
    ipos++;


    //! calculations of event lenght
    len = buffersize /4;
    sevt->sub_padding = ipos+len;
    len += len%2;
    sevt->sub_length += ipos+len;
    blockCounter++;

    if (fMAX_NEVT_FIFO_SEND!=0){
        if (blockCounter-fifocal_prevEventCounter>fMAX_NEVT_FIFO_SEND){
            addfifo(blockCounter);
            fifocal_prevEventCounter=blockCounter;
        }
    }

    //cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;
    return  sevt->sub_length;
}


int daq_device_caenadc::endrun()
{
    if ( _broken )
      {
        return 0; //  we had a catastrophic failure
      }
    return _broken;
}

void daq_device_caenadc::identify(std::ostream& os) const
{
    CAEN_DGTZ_BoardInfo_t       BoardInfo;
}



int daq_device_caenadc::max_length(const int etype) const
{
    if (etype != m_eventType) return 0;
    return  (14900);
}


// the rearm() function
int  daq_device_caenadc::rearm(const int etype)
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

void daq_device_caenadc::addfifo(int fifobufin)
{
    fifobuf=fifobufin;
    int fd = open(myfifo, O_WRONLY|O_NONBLOCK);
    write(fd, &fifobuf, sizeof(fifobuf));
    //printf("Send = %d\n",fifobuf);
}
