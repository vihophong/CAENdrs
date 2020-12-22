#include <iostream>

#include <daq_device_caen_v1730.h>
#include <string.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#include "fft.h"
#include "X742CorrectionRoutines.h"



extern int dc_file[MAX_CH];
extern int thr_file[MAX_CH];
int cal_ok[MAX_CH] = { 0 };

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

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


daq_device_caen_v1730::daq_device_caen_v1730(const int eventtype
                                             , const int subeventid
                                             , const int isdppmode // DPP mode (1) or waveform mode (0)
                                             , const int boardid
                                             , const int trigger):WDcfg(),WDrun()// do I give the system trigger?
{
    m_eventType  = eventtype;
    m_subeventid = subeventid;


    handle = 0;

    int node = 0;

    _warning = 0;

    ret = CAEN_DGTZ_Success;
    ErrCode= ERR_NONE;
    Nb=0, Ne=0;
    buffer = NULL;
    EventPtr = NULL;
    isVMEDevice= 0;
    nCycles= 0;

    Event16=NULL; /* generic event struct with 16 bit data (10, 12, 14 and 16 bit digitizers */

    Event8=NULL; /* generic event struct with 8 bit data (only for 8 bit digitizers) */
    Event742=NULL;  /* custom event struct with 8 bit data (only for 8 bit digitizers) */
    FILE *f_ini;

    ReloadCfgStatus = 0x7FFFFFFF; // Init to the bigger positive number

    /* *************************************************************************************** */
    /* Open and parse default configuration file                                                       */
    /* *************************************************************************************** */
    memset(&WDrun, 0, sizeof(WDrun));
    memset(&WDcfg, 0, sizeof(WDcfg));

    strcpy(ConfigFileName, DEFAULT_CONFIG_FILE);
    printf("Opening Configuration File %s\n", ConfigFileName);
    f_ini = fopen(ConfigFileName, "r");
    if (f_ini == NULL) {
        ErrCode = ERR_CONF_FILE_NOT_FOUND;
        exit(0);
    }
    ParseConfigFile(f_ini, &WDcfg);
    fclose(f_ini);

    /* *************************************************************************************** */
    /* Open the digitizer and read the board information                                       */
    /* *************************************************************************************** */
    isVMEDevice = WDcfg.BaseAddress ? 1 : 0;

    ret = CAEN_DGTZ_OpenDigitizer((CAEN_DGTZ_ConnectionType)WDcfg.LinkType, WDcfg.LinkNum, WDcfg.ConetNode, WDcfg.BaseAddress, &handle);
    if (ret) {
        ErrCode = ERR_DGZ_OPEN;
        exit(0);
    }

    ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    if (ret) {
        ErrCode = ERR_BOARD_INFO_READ;
        exit(0);
    }
    printf("Connected to CAEN Digitizer Model %s\n", BoardInfo.ModelName);
    printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
    printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);

    // Check firmware rivision (DPP firmwares cannot be used with WaveDump */
    sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
    if (MajorNumber >= 128) {
        printf("This digitizer has a DPP firmware\n");
        ErrCode = ERR_INVALID_BOARD_TYPE;
        exit(0);
    }

    /* *************************************************************************************** */
    /* Check if the board needs a specific config file and parse it instead of the default one */
    /* *************************************************************************************** */

    int use_specific_file = 0;
    //Check if model x742 is in use --> use its specific configuration file
    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
            strcpy(ConfigFileName, "WaveDumpConfig_X742.txt");
            printf("\nWARNING: using configuration file %s specific for Board model X742.\nEdit this file if you want to modify the default settings.\n ", ConfigFileName);
            use_specific_file = 1;
    }//Check if model x740 is in use --> use its specific configuration file
    else if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) {
            strcpy(ConfigFileName, "WaveDumpConfig_X740.txt");
            printf("\nWARNING: using configuration file %s specific for Board model X740.\nEdit this file if you want to modify the default settings.\n ", ConfigFileName);
            use_specific_file = 1;
    }

    if (use_specific_file) {
            memset(&WDrun, 0, sizeof(WDrun));
            memset(&WDcfg, 0, sizeof(WDcfg));

            f_ini = fopen(ConfigFileName, "r");
            if (f_ini == NULL) {
                    ErrCode = ERR_CONF_FILE_NOT_FOUND;
                    exit(0);
            }
            ParseConfigFile(f_ini, &WDcfg);
            fclose(f_ini);
    }

    // Get Number of Channels, Number of bits, Number of Groups of the board */
    ret = GetMoreBoardInfo(handle, BoardInfo, &WDcfg);
    if (ret) {
        ErrCode = ERR_INVALID_BOARD_TYPE;
        exit(0);
    }


    //Check for possible board internal errors
    ret = CheckBoardFailureStatus(handle, BoardInfo);
    if (ret) {
        ErrCode = ERR_BOARD_FAILURE;
        exit(0);
    }

    //set default DAC calibration coefficients
    for (int i = 0; i < MAX_SET; i++) {
        WDcfg.DAC_Calib.cal[i] = 1;
        WDcfg.DAC_Calib.offset[i] = 0;
    }
    //load DAC calibration data (if present in flash)
    if (BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)//XX742 not considered
        Load_DAC_Calibration_From_Flash(handle, &WDcfg, BoardInfo);

    // Perform calibration (if needed).
    if (WDcfg.StartupCalibration)
        calibrate(handle, &WDrun, BoardInfo);

    // mask the channels not available for this model
    if ((BoardInfo.FamilyCode != CAEN_DGTZ_XX740_FAMILY_CODE) && (BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)){
        WDcfg.EnableMask &= (1<<WDcfg.Nch)-1;
    } else {
        WDcfg.EnableMask &= (1<<(WDcfg.Nch/8))-1;
    }
    if ((BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) && WDcfg.DesMode) {
        WDcfg.EnableMask &= 0xAA;
    }
    if ((BoardInfo.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) && WDcfg.DesMode) {
        WDcfg.EnableMask &= 0x55;
    }
    // Set plot mask
    if ((BoardInfo.FamilyCode != CAEN_DGTZ_XX740_FAMILY_CODE) && (BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)){
        WDrun.ChannelPlotMask = WDcfg.EnableMask;
    } else {
        WDrun.ChannelPlotMask = (WDcfg.FastTriggerEnabled == 0) ? 0xFF: 0x1FF;
    }
    if ((BoardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE) || (BoardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE)) {
        WDrun.GroupPlotSwitch = 0;
    }

    /* *************************************************************************************** */
    /* program the digitizer                                                                   */
    /* *************************************************************************************** */
    ret = ProgramDigitizer(handle, WDcfg, BoardInfo);
    if (ret) {
        ErrCode = ERR_DGZ_PROGRAM;
        cout<<"Error while programming the Dititizer!"<<endl;
        exit(0);
    }
    usleep(300000);//300 ms

    //check for possible failures after programming the digitizer
    ret = CheckBoardFailureStatus(handle, BoardInfo);
    if (ret) {
            ErrCode = ERR_BOARD_FAILURE;
            cout<<"Board Failure detected!"<<endl;
            exit(0);
    }
    // Select the next enabled group for plotting
    if ((WDcfg.EnableMask) && (BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE || BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE))
        if( ((WDcfg.EnableMask>>WDrun.GroupPlotIndex)&0x1)==0 )
            GoToNextEnabledGroup(&WDrun, &WDcfg);
    // Read again the board infos, just in case some of them were changed by the programming
    // (like, for example, the TSample and the number of channels if DES mode is changed)
    if(ReloadCfgStatus > 0) {
        ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
        if (ret) {
            ErrCode = ERR_BOARD_INFO_READ;
            exit(0);
        }
        ret = GetMoreBoardInfo(handle,BoardInfo, &WDcfg);
        if (ret) {
            ErrCode = ERR_INVALID_BOARD_TYPE;
            exit(0);
        }

        // Reload Correction Tables if changed
        if(BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE && (ReloadCfgStatus & (0x1 << CFGRELOAD_CORRTABLES_BIT)) ) {
            if(WDcfg.useCorrections != -1) { // Use Manual Corrections
                uint32_t GroupMask = 0;

                // Disable Automatic Corrections
                if ((ret = CAEN_DGTZ_DisableDRS4Correction(handle)) != CAEN_DGTZ_Success)
                    exit(0);

                // Load the Correction Tables from the Digitizer flash
                if ((ret = CAEN_DGTZ_GetCorrectionTables(handle, WDcfg.DRS4Frequency, (void*)X742Tables)) != CAEN_DGTZ_Success)
                    exit(0);

                if(WDcfg.UseManualTables != -1) { // The user wants to use some custom tables
                    uint32_t gr;
                                        int32_t clret;

                    GroupMask = WDcfg.UseManualTables;

                    for(gr = 0; gr < WDcfg.MaxGroupNumber; gr++) {
                        if (((GroupMask>>gr)&0x1) == 0)
                            continue;
                        if ((clret = LoadCorrectionTable(WDcfg.TablesFilenames[gr], &(X742Tables[gr]))) != 0)
                            printf("Error [%d] loading custom table from file '%s' for group [%u].\n", clret, WDcfg.TablesFilenames[gr], gr);
                    }
                }
                // Save to file the Tables read from flash
                GroupMask = (~GroupMask) & ((0x1<<WDcfg.MaxGroupNumber)-1);
                char tmpchar[50];
                sprintf(tmpchar,"X742Table");
                SaveCorrectionTables(tmpchar, GroupMask, X742Tables);
            }
            else { // Use Automatic Corrections
                if ((ret = CAEN_DGTZ_LoadDRS4CorrectionData(handle, WDcfg.DRS4Frequency)) != CAEN_DGTZ_Success)
                    exit(0);
                if ((ret = CAEN_DGTZ_EnableDRS4Correction(handle)) != CAEN_DGTZ_Success)
                    exit(0);
            }
        }
    }


    // Allocate memory for the event data and readout buffer
    if(WDcfg.Nbit == 8)
        ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event8);
    else {
        if (BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE) {
            ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);
        }
        else {
            ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event742);
        }
    }
    if (ret != CAEN_DGTZ_Success) {
        ErrCode = ERR_MALLOC;
        exit(0);
    }
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer,&AllocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
    if (ret) {
        ErrCode = ERR_MALLOC;
        exit(0);
    }

    if (BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)//XX742 not considered
            Set_relative_Threshold(handle, &WDcfg, BoardInfo);


    if (trigger)   _trigger_handler=1;
    _trigger_handler=1;
    if ( _trigger_handler )
      {
        cout << __LINE__ << "  " << __FILE__ << " registering triggerhandler " << endl;
        _th  = new CAENdrsTriggerHandler (handle, m_eventType);
        registerTriggerHandler ( _th);
      }
    else
      {
        _th = 0;
      }
}



daq_device_caen_v1730::~daq_device_caen_v1730()
{
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    CAEN_DGTZ_FreeEvent(handle, (void**)&Event8);
    if(WDcfg.Nbit == 8)
        CAEN_DGTZ_FreeEvent(handle, (void**)&Event8);
    else
        if (BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE) {
            CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);
        }
        else {
            CAEN_DGTZ_FreeEvent(handle, (void**)&Event742);
        }
    if (_th)
    {
        clearTriggerHandler();
        delete _th;
        _th = 0;
    }
}

int  daq_device_caen_v1730::init()
{
    CAEN_DGTZ_SWStartAcquisition(handle);
    return 0;
}

// the put_data function

int daq_device_caen_v1730::put_data(const int etype, int * adr, const int length )
{
    //cout<<"put data"<<endl;
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

    /* Read data from the board */
    ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
    if (ret) {
        ErrCode = ERR_READOUT;
        cout<<"CAEN_DGTZ_ReadData Error!"<<endl;
        exit(0);
    }
    NumEvents = 0;
    if (BufferSize != 0) {
        ret = CAEN_DGTZ_GetNumEvents(handle, buffer, BufferSize, &NumEvents);
        if (ret) {
            ErrCode = ERR_READOUT;
            cout<<"CAEN_DGTZ_GetNumEvents Error!"<<endl;
            exit(0);
        }
    }
    else {
        uint32_t lstatus;
        ret = CAEN_DGTZ_ReadRegister(handle, CAEN_DGTZ_ACQ_STATUS_ADD, &lstatus);
        if (ret) {
            printf("Warning: Failure reading reg:%x (%d)\n", CAEN_DGTZ_ACQ_STATUS_ADD, ret);
        }
        else {
            if (lstatus & (0x1 << 19)) {
                ErrCode = ERR_OVERTEMP;
                cout<<"ERR_OVERTEMP code!"<<endl;
            }
        }
        return 0;
    }
    //! Calculate data
    //        ret = CAEthouroughput and rate
    Nb += BufferSize;
    Ne += NumEvents;
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PrevRateTime;
    nCycles++;
    if (ElapsedTime > 1000) {
        ReadoutRate=(float)Nb/((float)ElapsedTime*1048.576f);
        TriggerRate=(float)Ne*1000.0f/(float)ElapsedTime;
        if (Nb == 0)
            printf("No data...\n");
        else
            printf("Reading at %.2f MB/s (Trg Rate: %.2f Hz)\n", ReadoutRate, TriggerRate);

        nCycles= 0;
        Nb = 0;
        Ne = 0;
        PrevRateTime = CurrentTime;
    }


    len = BufferSize /4;

    sevt->sub_padding = len%2;
    len += sevt->sub_padding;
    sevt->sub_length += len;
    //cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;

    return  sevt->sub_length;
}


int daq_device_caen_v1730::endrun()
{
    CAEN_DGTZ_SWStopAcquisition(handle);
    if ( _broken )
      {
        return 0; //  we had a catastrophic failure
      }
    return _broken;
}

void daq_device_caen_v1730::identify(std::ostream& os) const
{
    CAEN_DGTZ_BoardInfo_t       BoardInfo;
}



int daq_device_caen_v1730::max_length(const int etype) const
{
    if (etype != m_eventType) return 0;
    //return  (14900);
    return  (14900);
}


// the rearm() function
int  daq_device_caen_v1730::rearm(const int etype)
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
