
#include <iostream>
#include <cstdlib>

#include <daq_device_caen_v1730.h>
#include <string.h>
#include <math.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

// important to be setup!
#define MAX_CH_1740_4DAQ 64
#define DEFAULT_THR_LOW_1740 200

// base line mean for Zero suspresion(for wave dump) 2^(3+1) = 16 samples, set minus value to disable Zero suspression!


using namespace std;


daq_device_caen_v1730::daq_device_caen_v1730(const int eventtype
                           , const int subeventid
                           , const int isdppmode // DPP mode (1) or waveform mode (0)
                           , const int boardid
                           , const int trigger)// do I give the system trigger?
{
  m_eventType  = eventtype;
  m_subeventid = subeventid;

  _isdppmode = isdppmode;
  if (_isdppmode==0) cout<<"Configuring for the WaveDump mode!\t"<<_isdppmode<<endl;
  else cout<<"Configuring for the DPP-PSD mode!\t"<<_isdppmode<<endl;

  for (int i=0;i<MAX_N_BOARD;i++) for (int j=0;j<MAX_N_CHANNEL;j++)
      trgcnt[i][j] = 0;

  handle = 0;
  _Event742 = 0;

  //modified by Phong
  b = boardid;
  _warning = 0;
  ret=0;
  if (_isdppmode==0){
      char conf[500];
      sprintf(conf,"m%i_wd.txt",b);
      cout<<"Configuration file for V1730 wavedump mode"<<conf<<endl;
      Evt = NULL;
      buffer = NULL;
      evtptr = NULL;
      ProgramCAEN1740(conf,trigger);  
  }else{
      ProgramCAEN1730(b,trigger);
  }
  //! check if zero encoding mode is enabled!
  ZS_nsbl = -1;
  ZS_pol = CAEN_DGTZ_PulsePolarityNegative;
  const char* env_bl = std::getenv("ZS_BASELINE");
  const char* env_pol = std::getenv("ZS_POLARITY");
  if(env_bl&&env_pol) {
      cout<<"Software Zero suspression mode enabled!"<<endl;
      ZS_nsbl = atoi(env_bl);
      cout<<"baseline mean= "<<ZS_nsbl<<endl;
      int zspolflag = atoi(env_pol);
      if (zspolflag!=0){
          ZS_pol = CAEN_DGTZ_PulsePolarityPositive;
          cout<<"common POSITIVE polarity"<<endl;
      }else{
          cout<<"common NEGATIVE polarity"<<endl;
      }
  }else{
      cout<<"Zero suspression mode disabled!"<<endl;
  }
  CAEN_DGTZ_SWStartAcquisition(handle);
}



void daq_device_caen_v1730::SetDefaultConfiguration(WaveDumpConfig_t *WDcfg) {
    int i, j;

    WDcfg->RecordLength = (1024*16);
    WDcfg->PostTrigger = 80;
    WDcfg->NumEvents = 1023;
    WDcfg->EnableMask = 0xFFFF;
    WDcfg->GWn = 0;
    WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
    WDcfg->InterruptNumEvents = 0;
    WDcfg->TestPattern = 0;
    WDcfg->DecimationFactor = 1;
    WDcfg->TriggerEdge = CAEN_DGTZ_TriggerOnRisingEdge;
    WDcfg->DesMode = 0;
    WDcfg->FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    WDcfg->FastTriggerEnabled = 0;
    WDcfg->FPIOtype = CAEN_DGTZ_IOLevel_NIM ;
    strcpy(WDcfg->GnuPlotPath, GNUPLOT_DEFAULT_PATH);
    for(i=0; i<MAX_SET; i++) {
        WDcfg->DCoffset[i] = 0;
        WDcfg->Threshold[i] = 0;
        WDcfg->ChannelTriggerMode[i] = CAEN_DGTZ_TRGMODE_DISABLED;
        WDcfg->GroupTrgEnableMask[i] = 0;
        for(j=0; j<MAX_SET; j++) WDcfg->DCoffsetGrpCh[i][j] = -1;
        WDcfg->FTThreshold[i] = 0;
        WDcfg->FTDCoffset[i] =0;
    }
    WDcfg->useCorrections = -1;
    WDcfg->UseManualTables = -1;
    for(i=0; i<MAX_X742_GROUP_SIZE; i++)
        sprintf(WDcfg->TablesFilenames[i], "Tables_gr%d", i);
    WDcfg->DRS4Frequency = CAEN_DGTZ_DRS4_5GHz;
}

/*! \fn      int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg)
*   \brief   Read the configuration file and set the WaveDump paremeters
*
*   \param   f_ini        Pointer to the config file
*   \param   WDcfg:   Pointer to the WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes; positive numbers
*               decodes the changes which need to perform internal parameters
*               recalculations.
*/


int daq_device_caen_v1730::GetMoreBoardInfo(int handle, CAEN_DGTZ_BoardInfo_t BoardInfo, WaveDumpConfig_t *WDcfg)
{
    int ret;
    switch(BoardInfo.FamilyCode) {
        CAEN_DGTZ_DRS4Frequency_t freq;

    case CAEN_DGTZ_XX724_FAMILY_CODE:
    case CAEN_DGTZ_XX781_FAMILY_CODE:
        WDcfg->Nbit = 14; WDcfg->Ts = 10.0; break;
    case CAEN_DGTZ_XX720_FAMILY_CODE: WDcfg->Nbit = 12; WDcfg->Ts = 4.0;  break;
    case CAEN_DGTZ_XX721_FAMILY_CODE: WDcfg->Nbit =  8; WDcfg->Ts = 2.0;  break;
    case CAEN_DGTZ_XX731_FAMILY_CODE: WDcfg->Nbit =  8; WDcfg->Ts = 2.0;  break;
    case CAEN_DGTZ_XX751_FAMILY_CODE: WDcfg->Nbit = 10; WDcfg->Ts = 1.0;  break;
    case CAEN_DGTZ_XX761_FAMILY_CODE: WDcfg->Nbit = 10; WDcfg->Ts = 0.25;  break;
    case CAEN_DGTZ_XX740_FAMILY_CODE: WDcfg->Nbit = 12; WDcfg->Ts = 16.0; break;
    case CAEN_DGTZ_XX730_FAMILY_CODE: WDcfg->Nbit = 14; WDcfg->Ts = 2.0; break;
    case CAEN_DGTZ_XX742_FAMILY_CODE:
        WDcfg->Nbit = 12;
        if ((ret = CAEN_DGTZ_GetDRS4SamplingFrequency(handle, &freq)) != CAEN_DGTZ_Success) return CAEN_DGTZ_CommError;
        switch (freq) {
        case CAEN_DGTZ_DRS4_1GHz:
            WDcfg->Ts = 1.0;
            break;
        case CAEN_DGTZ_DRS4_2_5GHz:
            WDcfg->Ts = (float)0.4;
            break;
        case CAEN_DGTZ_DRS4_5GHz:
            WDcfg->Ts = (float)0.2;
            break;
        }
        switch(BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->MaxGroupNumber = 4;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
        default:
            WDcfg->MaxGroupNumber = 2;
            break;
        }
        break;
    default: return -1;
    }
    if (((BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) ||
        (BoardInfo.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) ) && WDcfg->DesMode)
        WDcfg->Ts /= 2;

    switch(BoardInfo.FamilyCode) {
    case CAEN_DGTZ_XX724_FAMILY_CODE:
    case CAEN_DGTZ_XX781_FAMILY_CODE:
    case CAEN_DGTZ_XX720_FAMILY_CODE:
    case CAEN_DGTZ_XX721_FAMILY_CODE:
    case CAEN_DGTZ_XX751_FAMILY_CODE:
    case CAEN_DGTZ_XX761_FAMILY_CODE:
    case CAEN_DGTZ_XX731_FAMILY_CODE:
        switch(BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 8;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 4;
            break;
        }
        break;
    case CAEN_DGTZ_XX730_FAMILY_CODE:
        switch(BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 16;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 8;
            break;
        }
        break;
    case CAEN_DGTZ_XX740_FAMILY_CODE:
        switch( BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 64;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 32;
            break;
        }
        break;
    case CAEN_DGTZ_XX742_FAMILY_CODE:
        switch( BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 36;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 16;
            break;
        }
        break;
    default:
        return -1;
    }
    return 0;
}

int daq_device_caen_v1730::ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg)
{
    char str[1000], str1[1000], *pread;
    int i, ch=-1, val, Off=0, tr = -1;
    int ret = 0;

    // Save previous valus (for compare)
    int PrevDesMode = WDcfg->DesMode;
    int PrevUseCorrections = WDcfg->useCorrections;
    int PrevUseManualTables = WDcfg->UseManualTables;
    size_t TabBuf[sizeof(WDcfg->TablesFilenames)];
    // Copy the filenames to watch for changes
    memcpy(TabBuf, WDcfg->TablesFilenames, sizeof(WDcfg->TablesFilenames));

    /* Default settings */
    SetDefaultConfiguration(WDcfg);

    /* read config file and assign parameters */
    while(!feof(f_ini)) {
        int read;
        char *res;
        // read a word from the file
        read = fscanf(f_ini, "%s", str);
        if( !read || (read == EOF) || !strlen(str))
            continue;
        // skip comments
        if(str[0] == '#') {
            res = fgets(str, 1000, f_ini);
            continue;
        }

        if (strcmp(str, "@ON")==0) {
            Off = 0;
            continue;
        }
        if (strcmp(str, "@OFF")==0)
            Off = 1;
        if (Off)
            continue;


        // Section (COMMON or individual channel)
        if (str[0] == '[') {
            if (strstr(str, "COMMON")) {
                ch = -1;
               continue;
            }
            if (strstr(str, "TR")) {
                sscanf(str+1, "TR%d", &val);
                 if (val < 0 || val >= MAX_SET) {
                    printf("%s: Invalid channel number\n", str);
                } else {
                    tr = val;
                }
            } else {
                sscanf(str+1, "%d", &val);
                if (val < 0 || val >= MAX_SET) {
                    printf("%s: Invalid channel number\n", str);
                } else {
                    ch = val;
                }
            }
            continue;
        }

        // OPEN: read the details of physical path to the digitizer
        if (strstr(str, "OPEN")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "USB")==0)
                WDcfg->LinkType = CAEN_DGTZ_USB;
            else if (strcmp(str1, "PCI")==0)
                WDcfg->LinkType = CAEN_DGTZ_OpticalLink;
            else {
                printf("%s %s: Invalid connection type\n", str, str1);
                return -1;
            }
            read = fscanf(f_ini, "%d", &WDcfg->LinkNum);
            if (WDcfg->LinkType == CAEN_DGTZ_USB)
                WDcfg->ConetNode = 0;
            else
                read = fscanf(f_ini, "%d", &WDcfg->ConetNode);
            read = fscanf(f_ini, "%x", &WDcfg->BaseAddress);
            continue;
        }

        // Generic VME Write (address offset + data + mask, each exadecimal)
        if ((strstr(str, "WRITE_REGISTER")!=NULL) && (WDcfg->GWn < MAX_GW)) {
            read = fscanf(f_ini, "%x", (int *)&WDcfg->GWaddr[WDcfg->GWn]);
            read = fscanf(f_ini, "%x", (int *)&WDcfg->GWdata[WDcfg->GWn]);
            read = fscanf(f_ini, "%x", (int *)&WDcfg->GWmask[WDcfg->GWn]);
            WDcfg->GWn++;
            continue;
        }

        // Acquisition Record Length (number of samples)
        if (strstr(str, "RECORD_LENGTH")!=NULL) {
            read = fscanf(f_ini, "%d", &WDcfg->RecordLength);
            continue;
        }

        // Acquisition Record Length (number of samples)
        if (strstr(str, "DRS4_FREQUENCY")!=NULL) {
            int PrevDRS4Freq = WDcfg->DRS4Frequency;
            int freq;
            read = fscanf(f_ini, "%d", &freq);
            WDcfg->DRS4Frequency = (CAEN_DGTZ_DRS4Frequency_t)freq;
            if(PrevDRS4Freq != WDcfg->DRS4Frequency)
                ret |= (0x1 << CFGRELOAD_CORRTABLES_BIT);
            continue;
        }

        // Correction Level (mask)
        if (strstr(str, "CORRECTION_LEVEL")!=NULL) {
            int changed = 0;

            read = fscanf(f_ini, "%s", str1);
            if( strcmp(str1, "AUTO") == 0 )
                WDcfg->useCorrections = -1;
            else {
                int gr = 0;
                char Buf[1000];
                const char *tokens = " \t";
                char *ptr = Buf;

                WDcfg->useCorrections = atoi(str1);
                pread = fgets(Buf, 1000, f_ini); // Get the remaining line
                WDcfg->UseManualTables = -1;
                if(sscanf(ptr, "%s", str1) == 0) {
                    printf("Invalid syntax for parameter %s\n", str);
                    continue;
                }
                if(strcmp(str1, "AUTO") != 0) { // The user wants to use custom correction tables
                    WDcfg->UseManualTables = atoi(ptr); // Save the group mask
                    ptr = strstr(ptr, str1);
                    ptr += strlen(str1);
                    while(sscanf(ptr, "%s", str1) == 1 && gr < MAX_X742_GROUP_SIZE) {
                        while( ((WDcfg->UseManualTables) & (0x1 << gr)) == 0 && gr < MAX_X742_GROUP_SIZE)
                            gr++;
                        if(gr >= MAX_X742_GROUP_SIZE) {
                            printf("Error parsing values for parameter %s\n", str);
                            continue;
                        }
                        ptr = strstr(ptr, str1);
                        ptr += strlen(str1);
                        strcpy(WDcfg->TablesFilenames[gr], str1);
                        gr++;
                    }
                }
            }

            // Check for changes
            if (PrevUseCorrections != WDcfg->useCorrections)
                changed = 1;
            else if (PrevUseManualTables != WDcfg->UseManualTables)
                changed = 1;
            else if (memcmp(TabBuf, WDcfg->TablesFilenames, sizeof(WDcfg->TablesFilenames)))
                changed = 1;
            if (changed == 1)
                ret |= (0x1 << CFGRELOAD_CORRTABLES_BIT);
            continue;
        }

        // Test Pattern
        if (strstr(str, "TEST_PATTERN")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0)
                WDcfg->TestPattern = 1;
            else if (strcmp(str1, "NO")!=0)
                printf("%s: invalid option\n", str);
            continue;
        }

        // Acquisition Record Length (number of samples)
        if (strstr(str, "DECIMATION_FACTOR")!=NULL) {
            read = fscanf(f_ini, "%d", &WDcfg->DecimationFactor);
            continue;
        }

        // Trigger Edge
        if (strstr(str, "TRIGGER_EDGE")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "FALLING")==0)
                WDcfg->TriggerEdge =  CAEN_DGTZ_TriggerOnFallingEdge;
            else if (strcmp(str1, "RISING")!=0)
                printf("%s: invalid option\n", str);
            continue;
        }

        // External Trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
        if (strstr(str, "EXTERNAL_TRIGGER")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "DISABLED")==0)
                WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
            else if (strcmp(str1, "ACQUISITION_ONLY")==0)
                WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
            else if (strcmp(str1, "ACQUISITION_AND_TRGOUT")==0)
                WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
            else
                printf("%s: Invalid Parameter\n", str);
            continue;
        }

        // Max. number of events for a block transfer (0 to 1023)
        if (strstr(str, "MAX_NUM_EVENTS_BLT")!=NULL) {
            read = fscanf(f_ini, "%d", &WDcfg->NumEvents);
            continue;
        }

        // GNUplot path
        if (strstr(str, "GNUPLOT_PATH")!=NULL) {
            read = fscanf(f_ini, "%s", WDcfg->GnuPlotPath);
            continue;
        }

        // Post Trigger (percent of the acquisition window)
        if (strstr(str, "POST_TRIGGER")!=NULL) {
            read = fscanf(f_ini, "%d", &WDcfg->PostTrigger);
            continue;
        }

        // DesMode (Double sampling frequency for the Mod 731 and 751)
        if (strstr(str, "ENABLE_DES_MODE")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0)
                WDcfg->DesMode = 1;
            else if (strcmp(str1, "NO")!=0)
                printf("%s: invalid option\n", str);
            if(PrevDesMode != WDcfg->DesMode)
                ret |= (0x1 << CFGRELOAD_DESMODE_BIT);
            continue;
        }

        // Output file format (BINARY or ASCII)
        if (strstr(str, "OUTPUT_FILE_FORMAT")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "BINARY")==0)
                WDcfg->OutFileFlags = OFF_BINARY;
            else if (strcmp(str1, "ASCII")!=0)
                printf("%s: invalid output file format\n", str1);
            continue;
        }

        // Header into output file (YES or NO)
        if (strstr(str, "OUTPUT_FILE_HEADER")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0)
                WDcfg->OutFileFlags = OFF_HEADER;
            else if (strcmp(str1, "NO")!=0)
                printf("%s: invalid option\n", str);
            continue;
        }

        // Interrupt settings (request interrupt when there are at least N events to read; 0=disable interrupts (polling mode))
        if (strstr(str, "USE_INTERRUPT")!=NULL) {
            read = fscanf(f_ini, "%d", &WDcfg->InterruptNumEvents);
            continue;
        }

        if (!strcmp(str, "FAST_TRIGGER")) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "DISABLED")==0)
                WDcfg->FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
            else if (strcmp(str1, "ACQUISITION_ONLY")==0)
                WDcfg->FastTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
            else
                printf("%s: Invalid Parameter\n", str);
            continue;
        }

        if (strstr(str, "ENABLED_FAST_TRIGGER_DIGITIZING")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0)
                WDcfg->FastTriggerEnabled= 1;
            else if (strcmp(str1, "NO")!=0)
                printf("%s: invalid option\n", str);
            continue;
        }

        // DC offset (percent of the dynamic range, -50 to 50)
        if (!strcmp(str, "DC_OFFSET")) {
            float dc;
            read = fscanf(f_ini, "%f", &dc);
            if (tr != -1) {
// 				WDcfg->FTDCoffset[tr] = dc;
                WDcfg->FTDCoffset[tr*2] = (uint32_t)dc;
                WDcfg->FTDCoffset[tr*2+1] = (uint32_t)dc;
                continue;
            }
            val = (int)((dc+50) * 65535 / 100);
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    WDcfg->DCoffset[i] = val;
            else
                WDcfg->DCoffset[ch] = val;
            continue;
        }

        if (strstr(str, "GRP_CH_DC_OFFSET")!=NULL) {
            float dc[8];
            read = fscanf(f_ini, "%f,%f,%f,%f,%f,%f,%f,%f", &dc[0], &dc[1], &dc[2], &dc[3], &dc[4], &dc[5], &dc[6], &dc[7]);
            for(i=0; i<8; i++) {
                val = (int)((dc[i]+50) * 65535 / 100);
                WDcfg->DCoffsetGrpCh[ch][i] = val;
            }
            continue;
        }

        // Threshold
        if (strstr(str, "TRIGGER_THRESHOLD")!=NULL) {
            read = fscanf(f_ini, "%d", &val);
            if (tr != -1) {
//				WDcfg->FTThreshold[tr] = val;
                WDcfg->FTThreshold[tr*2] = val;
                WDcfg->FTThreshold[tr*2+1] = val;

                continue;
            }
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    WDcfg->Threshold[i] = val;
            else
                WDcfg->Threshold[ch] = val;
            continue;
        }

        // Group Trigger Enable Mask (hex 8 bit)
        if (strstr(str, "GROUP_TRG_ENABLE_MASK")!=NULL) {
            read = fscanf(f_ini, "%x", &val);
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    WDcfg->GroupTrgEnableMask[i] = val & 0xFF;
            else
                 WDcfg->GroupTrgEnableMask[ch] = val & 0xFF;
            continue;
        }

        // Channel Auto trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
        if (strstr(str, "CHANNEL_TRIGGER")!=NULL) {
            CAEN_DGTZ_TriggerMode_t tm;
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "DISABLED")==0)
                tm = CAEN_DGTZ_TRGMODE_DISABLED;
            else if (strcmp(str1, "ACQUISITION_ONLY")==0)
                tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
            else if (strcmp(str1, "ACQUISITION_AND_TRGOUT")==0)
                tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
            else {
                printf("%s: Invalid Parameter\n", str);
                continue;
            }
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    WDcfg->ChannelTriggerMode[i] = tm;
            else
                WDcfg->ChannelTriggerMode[ch] = tm;
            continue;
        }

        // Front Panel LEMO I/O level (NIM, TTL)
        if (strstr(str, "FPIO_LEVEL")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "TTL")==0)
                WDcfg->FPIOtype = CAEN_DGTZ_IOLevel_TTL;
            else if (strcmp(str1, "NIM")!=0)
                printf("%s: invalid option\n", str);
            continue;
        }

        // Channel Enable (or Group enable for the V1740) (YES/NO)
        if (strstr(str, "ENABLE_INPUT")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0) {
                if (ch == -1)
                    WDcfg->EnableMask = 0xFF;
                else
                    WDcfg->EnableMask |= (1 << ch);
                continue;
            } else if (strcmp(str1, "NO")==0) {
                if (ch == -1)
                    WDcfg->EnableMask = 0x00;
                else
                    WDcfg->EnableMask &= ~(1 << ch);
                continue;
            } else {
                printf("%s: invalid option\n", str);
            }
            continue;
        }

        printf("%s: invalid setting\n", str);
    }
    return ret;
}



void daq_device_caen_v1730::ProgramCAEN1740(char* cfg,int trigger){
    FILE *f_ini;
    f_ini = fopen(cfg, "r");
    printf("Loading configuration file: %s\n",cfg);    
    ParseConfigFile(f_ini, &WDcfg);
    fclose(f_ini);
    ret = CAEN_DGTZ_OpenDigitizer(WDcfg.LinkType, WDcfg.LinkNum, WDcfg.ConetNode, WDcfg.BaseAddress, &handle);    
    cout<<"\nLINK number= "<<WDcfg.LinkNum<<"\nBOARD number= "<<WDcfg.LinkNum<<endl;
    if(ret != CAEN_DGTZ_Success) {
        printf("Can't open digitizer\n");
        exit(0);
    }
    /* Once we have the handler to the digitizer, we use it to call the other functions */
    ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    printf("\nConnected to CAEN Digitizer Model %s, recognized as board %d\n", BoardInfo.ModelName, b);
    printf("\tROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
    printf("\tAMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);

    // Get Number of Channels, Number of bits, Number of Groups of the board */
    ret = GetMoreBoardInfo(handle, BoardInfo, &WDcfg);
    if (ret) {
        printf("Error!\n");
        exit(0);
    }
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
    ret |= Program1740Digitizer(handle,WDcfg,BoardInfo);
    if (ret) {
        exit(0);
    }
    ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Evt);

    _trigger_handler=0;
    if (trigger)   _trigger_handler=1;

    _speed = CAEN_DGTZ_DRS4_1GHz;


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



void daq_device_caen_v1730::ProgramCAEN1730(int b,int trigger){
    //load parms
    sprintf(configfile,"m%i.conf",b);
    cout<<configfile<<endl;
    dgtz_parms[b]=getDigitizerParams(configfile);
    dpp_parms[b]=getDPPParams(configfile);
    cout << "*************** opening Digitizer" << endl;
    //OPEN digitizer, get handle
    ret = CAEN_DGTZ_OpenDigitizer(dgtz_parms[b].LinkType, dgtz_parms[b].LinkNum, dgtz_parms[b].BoardNum, dgtz_parms[b].VMEBaseAddress, &handle);
    if (ret) {
      printf("Can not open digitizer!\n");
    }
    /* Once we have the handler to the digitizer, we use it to call the other functions */
    ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    if (ret) {
      printf("Can't read board info\n");
      exit(0);
    }
    printf("PPP Connected to CAEN Digitizer Model %s as board %i \nROC FPGA Release is %s\nAMC FPGA Release is %s\n ------------\n",
          BoardInfo.ModelName, b,BoardInfo.ROC_FirmwareRel,BoardInfo.AMC_FirmwareRel);

    // Check firmware revision (only DPP firmware can be used with this Demo) */
   int MajorNumber;
    sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
    if (MajorNumber != 131 && MajorNumber != 132&& MajorNumber != 136 ) {
      printf("This digitizer has not a DPP-PSD firmware\n");
    }
    _broken =  CAEN_DGTZ_Reset(handle);

    if ( _broken )
      {
        cout << " Error in CAEN_DGTZ_OpenDigitizer " << _broken << endl;
        return;
      }

    _trigger_handler=0;
    if (trigger)   _trigger_handler=1;


    _speed = CAEN_DGTZ_DRS4_1GHz;


    //! Program digitizer here
    _broken = ProgramDigitizer(handle, dgtz_parms[b], dpp_parms[b],configfile);
    if ( _broken )
      {
        cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
        return;
      }
    //_broken =WriteRegisterBitmask(handle,0x8100, 0x4, 0x4);


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

void daq_device_caen_v1730::identify(std::ostream& os) const
{

  if ( _broken)
    {
      os << "CAEN 1730 Digitizer Event Type: " << m_eventType
     << " Subevent id: " << m_subeventid
     << " ** not functional ** " << endl;
    }
  else
    {
      os << "CAEN 1742 Digitizer STD Model " << BoardInfo.ModelName
     << " Event Type: " << m_eventType
     << " Subevent id: " << m_subeventid
     << " Firmware "     << BoardInfo.ROC_FirmwareRel << " / " << BoardInfo.AMC_FirmwareRel;
      if (_trigger_handler) os << " *Trigger" ;
      if (_warning) os << " **** warning - check setup parameters ****";
      os << endl;

    }
}


daq_device_caen_v1730::~daq_device_caen_v1730()
{

  if (_th)
    {
      clearTriggerHandler();
      delete _th;
      _th = 0;
    }

  /*
  if ( _Event742)
    {
      CAEN_DGTZ_FreeEvent(handle, (void**)&_Event742);
    }
  */

  CAEN_DGTZ_CloseDigitizer(handle);
  CAEN_DGTZ_FreeReadoutBuffer(&sbuffer_t);
  CAEN_DGTZ_FreeDPPEvents(handle, sEvents_t);
  CAEN_DGTZ_FreeDPPWaveforms(handle, sWaveform_t);
}


int  daq_device_caen_v1730::init()
{

  if ( _broken )
    {
      return 0; //  we had a catastrophic failure
    }


  if(b<100&&_isdppmode!=0){
      /* Allocate memory for the readout buffer */
      ret |= CAEN_DGTZ_MallocReadoutBuffer(handle, &sbuffer_t, &sAllocatedSize);
      /* Allocate memory for the events */
      ret |= CAEN_DGTZ_MallocDPPEvents(handle, sEvents_t, &sAllocatedSize);
      /* Allocate memory for the waveforms */
      ret |= CAEN_DGTZ_MallocDPPWaveforms(handle, &sWaveform_t, &sAllocatedSize);
      if (ret) {
          cout<<"Can't Allocate Memory Buffer!"<<endl;
          cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
          return 0;
      }

      for (int i=0;i<MAX_N_BOARD;i++) for (int j=0;j<MAX_N_CHANNEL;j++)
          trgcnt[i][j] = 0;

      // and we trigger rearm with our event type so it takes effect
      rearm (m_eventType);
      //! arm aquisition (for S-IN start mode only)
      //_broken =WriteRegisterBitmask(handle,0x8100, 0x4, 0x4);
      //! start aquisition (for normal software start)
      //_broken = CAEN_DGTZ_SWStartAcquisition(handle);

      if ( _broken )
        {
          cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
          return 0;
        }
  }else{//! v1740
      ret = CAEN_DGTZ_MallocReadoutBuffer(handle,&buffer,&sBufferSize);
      if (ret) {
          cout<<"Can't Allocate Memory Buffer!"<<endl;
          cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
          return 0;
      }
      for (int i=0;i<MAX_N_BOARD;i++) for (int j=0;j<MAX_N_CHANNEL;j++)
          trgcnt[i][j] = 0;

      // and we trigger rearm with our event type so it takes effect
      rearm (m_eventType);
      //! clear buffer(not working)
      //_broken =WriteRegisterBitmask(handle,0xEF28, 0x1, 0x1);

      //! arm aquisition (for S-IN start mode only)
      _broken =WriteRegisterBitmask(handle,0x8100, 0x4, 0x4);

      //! start aquisition (for normal software start)
      //_broken = CAEN_DGTZ_SWStartAcquisition(handle);
       if ( _broken )
        {
          cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
          return 0;
        }
  }
  return 0;

}

// the put_data function

int daq_device_caen_v1730::put_data(const int etype, int * adr, const int length )
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

    uint32_t sBufferSize = 0;

    if(_isdppmode!=0){
        //while ( sBufferSize == 0)
          //{
            // Read data from the board
            _broken = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, sbuffer_t, &sBufferSize);
            if ( _broken )
          {
            cout << __FILE__ << " " << __LINE__ << " _broken = " << _broken<< endl;
            return 1;
          }
            //      cout << __FILE__ << " " << __LINE__ << " buffersize = " << buffersize << endl;
          //}
        if (sBufferSize==0) return 0;

        uint32_t NumEvents_t[16];
        _broken =CAEN_DGTZ_GetDPPEvents(handle, sbuffer_t, sBufferSize, sEvents_t, NumEvents_t);
        if ( _broken )
              {
                cout << __FILE__ << " " << __LINE__ << " _broken = " << _broken<< endl;
                return 1;
              }
            Events_t= (CAEN_DGTZ_DPP_PSD_Event_t**) (sEvents_t);
            int ch,ev;
            //int evtbuff[16000];//max capacity=1000 event per all
            int ipos=0;
            int  *d=(int*) &sevt->data;

            for(ch=0; ch<MAX_N_CHANNEL; ch++) {
                //if (!(0x1 & (1<<ch))) continue;
                for(ev=0; ev<NumEvents_t[ch]; ev++){
                    //self trigger
                    //if (Events_t[ch][ev].ChargeLong<=0) continue;
                    sBufferSize++;
                    trgcnt[b][ch]++;
                    *d++ = b;
                    ipos++;
                    *d++ = ch;
                    ipos++;
                    *d++ = Events_t[ch][ev].TimeTag;
                    ipos++;
                    *d++ = (uint32_t) Events_t[ch][ev].ChargeShort;
                    ipos++;
                    *d++ = (uint32_t) Events_t[ch][ev].ChargeLong;
                    ipos++;
                    *d++ = Events_t[ch][ev].Extras;
                    ipos++;
                    *d++ = (uint32_t) Events_t[ch][ev].Baseline;
                    ipos++;
                    *d++ = trgcnt[b][ch];
                    ipos++;
                    uint16_t *WaveLine;
                    CAEN_DGTZ_DecodeDPPWaveforms(handle, &Events_t[ch][ev], sWaveform_t); //decode wf cost computing time
                    Waveform_t = (CAEN_DGTZ_DPP_PSD_Waveforms_t*)(sWaveform_t);
                    WaveLine = Waveform_t->Trace1; // Input Signal

                    //PULSE rejection start here
                    if (checkpulse(WaveLine,(int) Waveform_t->Ns,dpp_parms[b].nsbl[ch],(double)dpp_parms[b].thr[ch], dgtz_parms[b].PulsePolarity[ch])){
                        *d++ = (int) Waveform_t->Ns;
                        ipos++;
                        for (uint32_t i=0;i<Waveform_t->Ns/2;i++){
                              *d++ = combineint(WaveLine[i*2],WaveLine[i*2+1]);
                              ipos++;
                        }
                    }else{
                        int nstemp=(int) pow(2,dpp_parms[b].nsbl[ch]+1);
                        *d++ = nstemp;
                        ipos++;
                        for (uint32_t i=0;i<nstemp/2;i++){
                            *d++ = combineint(WaveLine[i*2],WaveLine[i*2+1]);
                            ipos++;
                        }
                    }
                    //Else
                    //*d++ = (int) Waveform_t->Ns;
                    //ipos++;
                    //for (uint32_t i=0;i<Waveform_t->Ns/2;i++){
                    //      *d++ = combineint(WaveLine[i*2],WaveLine[i*2+1]);
                    //      ipos++;
                    //}
                }
            }
            int padding=ipos%2;
            sevt->sub_padding = ipos;
            ipos=ipos+padding;
            sevt->sub_length += ipos;
            //cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;
    }else{
        // Read data from the board
        _broken = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, (char *) buffer, &sBufferSize);
        if ( _broken )
        {
            cout << __FILE__ << " " << __LINE__ << " _broken = " << _broken<< endl;
            return 1;
        }
        if (sBufferSize==0) {
            uint32_t lstatus;
            ret = CAEN_DGTZ_ReadRegister(handle, CAEN_DGTZ_ACQ_STATUS_ADD, &lstatus);
            if (lstatus & (0x1 << 19)) {
                cout<<__FILE__<<"Error found!"<<endl;
                exit(0);
            }
            return 0;
        }
        int ipos=0;
        int ipos_prev = 0;
        int  *d=(int*) &sevt->data;
        numEvents = 0;
        ret = CAEN_DGTZ_GetNumEvents(handle,buffer,sBufferSize,&numEvents);
        for (int i=0;i<(int)numEvents;i++) {
            ret = CAEN_DGTZ_GetEventInfo(handle,buffer,sBufferSize,i,&eventInfo,&evtptr);
            ret = CAEN_DGTZ_DecodeEvent(handle,evtptr,(void**)&Evt);
            //*************************************
            // Event Elaboration
            //*************************************
            *d++=eventInfo.BoardId;
            ipos++;
            *d++=eventInfo.ChannelMask;
            ipos++;
            *d++=eventInfo.EventCounter;
            ipos++;
            *d++=eventInfo.EventSize;
            ipos++;
            *d++=eventInfo.Pattern;
            ipos++;
            *d++=eventInfo.TriggerTimeTag;
            ipos++;
            if (ZS_nsbl>0&&b<102){ // zero suspression mode for v1740 , module 0 and 1(for WASABI)
                for (int i=0;i<MAX_CH_1740_4DAQ;i++){
                    //if (i==62&&b==100) cout<<"thr62="<<WDcfg.Threshold[i/8]<<endl;
                    if (!checkpositivepulse1740(Evt->DataChannel[i],Evt->ChSize[i],WDcfg.Threshold[i/8],DEFAULT_THR_LOW_1740)){
                        Evt->ChSize[i]=ZS_nsbl;
                    }
                }
            }
            memcpy(d++,Evt->ChSize,MAX_UINT16_CHANNEL_SIZE*sizeof(uint32_t));
            ipos+=MAX_UINT16_CHANNEL_SIZE;
            ipos_prev = MAX_UINT16_CHANNEL_SIZE -1; // -1 bit is nescessary!
            for (int i=0;i<MAX_CH_1740_4DAQ;i++){
                memcpy(d+=ipos_prev,Evt->DataChannel[i],Evt->ChSize[i]*sizeof(uint16_t));
                ipos_prev = Evt->ChSize[i]/2+Evt->ChSize[i]%2;
                ipos+=ipos_prev;
            }
            ret = CAEN_DGTZ_FreeEvent(handle,(void**)&Evt);
        }
        int padding=ipos%2;
        sevt->sub_padding = ipos;
        ipos=ipos+padding;
        sevt->sub_length += ipos;

        //cout<<"sub_length = "<<sevt->sub_length<<endl;
    }

    return  sevt->sub_length;
}



int daq_device_caen_v1730::endrun()
{
  if ( _broken )
    {
      return 0; //  we had a catastrophic failure
    }

  //  cout << __LINE__ << "  " << __FILE__ << " ending run " << endl;

  if(b<100&&_isdppmode!=0){
      //! disarm aquitition (for S-IN start only)
      //_broken =WriteRegisterBitmask(handle,0x8100, 0x0, 0x4);

      //! stop aquisition (for normal software start)
      //_broken = CAEN_DGTZ_SWStopAcquisition(handle);
      if ( _broken )
        {
          cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;

        }
  }else{
      //! disarm aquitition (for S-IN start only)
      //_broken =WriteRegisterBitmask(handle,0x8100, 0x0, 0x4);

      //! stop aquisition (for normal software start)
      //_broken = CAEN_DGTZ_SWStopAcquisition(handle);

      if ( _broken )
        {
          cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
        }

  }

  return _broken;

}



int daq_device_caen_v1730::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
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

int daq_device_caen_v1730::SetConfigRegisterBit( const int bit)
{
  const unsigned int forbidden_mask = 0x0FFFE7B7;  // none of these bits must be touched
  unsigned int pattern = 1<<bit;
  if ( pattern & forbidden_mask)
    {
      cout << " attemt to set reserved bit: " << bit << endl;
      return 1;
    }

  return CAEN_DGTZ_WriteRegister(handle,CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, pattern);
}

int daq_device_caen_v1730::ClearConfigRegisterBit( const int bit)
{
  const unsigned int forbidden_mask = 0x0FFFE7B7;  // none of these bits must be touched
  unsigned int pattern = 1<<bit;
  if ( pattern & forbidden_mask)
    {
      cout << " attemt to set reserved bit: " << bit << endl;
      return 1;
    }

  return CAEN_DGTZ_WriteRegister(handle,CAEN_DGTZ_BROAD_CH_CLEAR_CTRL_ADD, pattern);
}

int daq_device_caen_v1730::WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask) {
  int32_t ret = CAEN_DGTZ_Success;
  uint32_t d32 = 0xFFFFFFFF;

  ret = CAEN_DGTZ_ReadRegister(handle, address, &d32);
  if(ret != CAEN_DGTZ_Success)
    return ret;

  data &= mask;
  d32 &= ~mask;
  d32 |= data;
  ret = CAEN_DGTZ_WriteRegister(handle, address, d32);
  return ret;
}

CAEN_DGTZ_DPP_PSD_Params_t daq_device_caen_v1730::getDPPParams(char* filename)
{
   CAEN_DGTZ_DPP_PSD_Params_t DPPParams;
  FILE *infile;
  infile=fopen(filename,"r");
  char line[1000];
  if (infile==NULL) perror("Error opening config file:");
  int channel=-1;
  while (fgets(line,1000,infile)!=NULL){
    char ident[500];
    char var[500];
    sscanf(line,"%s %s",ident,var);
    if (strcmp(ident,"DPPParams.purgap")==0) DPPParams.purgap=atoi(var);
    if (strcmp(ident,"DPPParams.blthr")==0) DPPParams.blthr=atoi(var);
    if (strcmp(ident,"DPPParams.bltmo")==0) DPPParams.bltmo=atoi(var);
    if (strcmp(ident,"DPPParams.trgho")==0) DPPParams.trgho=atoi(var);
    if (strcmp(ident,"DPPParams.purh")==0){
      if (strcmp(var,"CAEN_DGTZ_DPP_PSD_PUR_DetectOnly")==0){
    DPPParams.purh = CAEN_DGTZ_DPP_PSD_PUR_DetectOnly;
      }else{
    DPPParams.purh = CAEN_DGTZ_DPP_PSD_PUR_Enabled;
      }
    }

    char abc[100];
    //    char xyz[100];
    if (line[0]=='['){
      sscanf(line,"[ %s ]",abc);
      //strncpy(xyz,abc,strlen(abc)-1);
      channel=atoi(abc);
    }
    if (strcmp(ident,"DPPParams.thr[ch]")==0) DPPParams.thr[channel]=atoi(var);
    if (strcmp(ident,"DPPParams.nsbl[ch]")==0) DPPParams.nsbl[channel]=atoi(var);
    if (strcmp(ident,"DPPParams.lgate[ch]")==0) DPPParams.lgate[channel]=atoi(var);
    if (strcmp(ident,"DPPParams.sgate[ch]")==0) DPPParams.sgate[channel]=atoi(var);
    if (strcmp(ident,"DPPParams.pgate[ch]")==0) DPPParams.pgate[channel]=atoi(var);
    if (strcmp(ident,"DPPParams.selft[ch]")==0) {
      DPPParams.selft[channel]=atoi(var);
    }
    if (strcmp(ident,"DPPParams.tvaw[ch]")==0) DPPParams.tvaw[channel]=atoi(var);
    if (strcmp(ident,"DPPParams.csens[ch]")==0) DPPParams.csens[channel]=atoi(var);
    if (strcmp(ident,"DPPParams.trgc[ch]")==0){
      if (strcmp(var,"CAEN_DGTZ_DPP_TriggerConfig_Threshold")==0){
    DPPParams.trgc[channel] = CAEN_DGTZ_DPP_TriggerConfig_Threshold;
      }else{
    DPPParams.trgc[channel] = CAEN_DGTZ_DPP_TriggerConfig_Peak;
      }
    }
  }
  /*
   int ch;
        for(ch=0; ch<MaxNChannels; ch++) {
            DPPParams.thr[ch] = 500;        // Trigger Threshold
            DPPParams.nsbl[ch] = 2;
            DPPParams.lgate[ch] = 32;    // Long Gate Width (N*4ns)
            DPPParams.sgate[ch] = 24;    // Short Gate Width (N*4ns)
            DPPParams.pgate[ch] = 8;     // Pre Gate Width (N*4ns)
            DPPParams.selft[ch] = 1;
            DPPParams.trgc[ch] = CAEN_DGTZ_DPP_TriggerConfig_Threshold;
            DPPParams.tvaw[ch] = 50;
            DPPParams.csens[ch] = 0;
        }
        DPPParams.purh = CAEN_DGTZ_DPP_PSD_PUR_DetectOnly;
        DPPParams.purgap = 100;  // Purity Gap
        DPPParams.blthr = 3;     // Baseline Threshold
        DPPParams.bltmo = 100;   // Baseline Timeout
        DPPParams.trgho = 8;     // Trigger HoldOff
  */
    return DPPParams;
}
DigitizerParams_t daq_device_caen_v1730::getDigitizerParams(char* filename)
{
  DigitizerParams_t Params;
  uint32_t Mask=0x0;
  FILE *infile;
  infile=fopen(filename,"r");
  char line[1000];
  if (infile==NULL) perror("Error opening config file:");
  int channel=-1;
  while (fgets(line,1000,infile)!=NULL){
    char ident[500];
    char var[500];
    sscanf(line,"%s %s",ident,var);
    if (strcmp(ident,"SAVE_DIR")==0) strcpy(Params.saveDir,var);
    if (strcmp(ident,"SAVE_NAME")==0) strcpy(Params.saveName,var);
    if (strcmp(ident,"Params.LinkType")==0){
      if (strcmp(var,"CAEN_DGTZ_OpticalLink")==0){
    Params.LinkType = CAEN_DGTZ_OpticalLink;
      }else{
    Params.LinkType = CAEN_DGTZ_USB;
      }
    }
    if (strcmp(ident,"Params.LinkNum")==0) Params.LinkNum=atoi(var);
    if (strcmp(ident,"Params.BoardNum")==0) Params.BoardNum=atoi(var);
    if (strcmp(ident,"Params.VMEBaseAddress")==0) Params.VMEBaseAddress=atoi(var);
    if (strcmp(ident,"Params.IOlev")==0){
      if (strcmp(var,"CAEN_DGTZ_IOLevel_TTL")==0){
     Params.IOlev = CAEN_DGTZ_IOLevel_TTL;
      }else{
    Params.IOlev = CAEN_DGTZ_IOLevel_NIM;
      }
    }
    if (strcmp(ident,"Params.AcqMode")==0){
      if (strcmp(var,"CAEN_DGTZ_DPP_ACQ_MODE_Mixed")==0){
    Params.AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;
      }else if (strcmp(var,"CAEN_DGTZ_DPP_ACQ_MODE_List")==0){
    Params.AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_List;
      }else{
    Params.AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope;
      }
    }
    if (strcmp(ident,"Params.RecordLength")==0) Params.RecordLength=atoi(var);
    if (strcmp(ident,"Params.EventAggr")==0) Params.EventAggr=atoi(var);
    if (strcmp(ident,"Params.ExtTriggerInputMode")==0){
      if (strcmp(var,"CAEN_DGTZ_TRGMODE_DISABLED")==0){
    Params.ExtTriggerInputMode = CAEN_DGTZ_TRGMODE_DISABLED;
      }else if (strcmp(var,"CAEN_DGTZ_TRGMODE_EXTOUT_ONLY")==0){
    Params.ExtTriggerInputMode = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      }else if (strcmp(var,"CAEN_DGTZ_TRGMODE_ACQ_ONLY")==0){
    Params.ExtTriggerInputMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      }else{
    Params.ExtTriggerInputMode= CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      }
    }
    char abc[100];
    char xyz[100];
    if (line[0]=='['){
      sscanf(line,"[%s]",abc);
      strncpy(xyz,abc,strlen(abc)-1);
      channel=atoi(xyz);
      //      printf("%d\n",channel);
    }
    if (strcmp(ident,"Params.ChannelEnabled[ch]")==0){
      if (strcmp(var,"YES")==0){
    Mask=Mask|(1<<channel);
      }
    }
    if (strcmp(ident,"Params.PulsePolarity[ch]")==0){
      if (strcmp(var,"CAEN_DGTZ_PulsePolarityNegative")==0){
    Params.PulsePolarity[channel]= CAEN_DGTZ_PulsePolarityNegative;
      }else{
    Params.PulsePolarity[channel]= CAEN_DGTZ_PulsePolarityPositive;
      }
    }
    if (strcmp(ident,"Params.PreTriggerSize[ch]")==0)  Params.PreTriggerSize[channel]=atoi(var);
    if (strcmp(ident,"Params.DCOffset[ch]")==0){
      Params.DCOffset[channel]= (int)((atof(var)+50) * 65535 / 100);
    }
  }

  Params.ChannelMask = Mask;                               // Channel enable mask
  return Params;
}


int daq_device_caen_v1730::Program1740Digitizer(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo)
{
    int i,j, ret = 0;

    /* reset the digitizer */
    ret |= CAEN_DGTZ_Reset(handle);
    if (ret != 0) {
        printf("Error: Unable to reset digitizer.\nPlease reset digitizer manually then restart the program\n");
        return -1;
    }

    // Set the waveform test bit for debugging
    if (WDcfg.TestPattern)
        ret |= CAEN_DGTZ_WriteRegister(handle, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);
    // custom setting for X742 boards
    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
        if (WDcfg.FastTriggerEnabled==1) ret |= CAEN_DGTZ_SetFastTriggerDigitizing(handle,CAEN_DGTZ_ENABLE );
        else ret |= CAEN_DGTZ_SetFastTriggerDigitizing(handle, CAEN_DGTZ_DISABLE);
        ret |= CAEN_DGTZ_SetFastTriggerMode(handle,WDcfg.FastTriggerMode);
    }
    if ((BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) || (BoardInfo.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE)) {
        if (WDcfg.DesMode==1) ret |= CAEN_DGTZ_SetDESMode(handle, CAEN_DGTZ_ENABLE);
        else ret |= CAEN_DGTZ_SetDESMode(handle, CAEN_DGTZ_DISABLE);
    }
    ret |= CAEN_DGTZ_SetRecordLength(handle, WDcfg.RecordLength);
    //ret |= CAEN_DGTZ_GetRecordLength(handle, &WDcfg.RecordLength);

    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) {
        ret |= CAEN_DGTZ_SetDecimationFactor(handle, WDcfg.DecimationFactor);
    }

    ret |= CAEN_DGTZ_SetPostTriggerSize(handle, WDcfg.PostTrigger);
    //if(BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE)
        //ret |= CAEN_DGTZ_GetPostTriggerSize(handle, &WDcfg.PostTrigger);
    ret |= CAEN_DGTZ_SetIOLevel(handle, WDcfg.FPIOtype);
    if( WDcfg.InterruptNumEvents > 0) {
        // Interrupt handling
        if( ret |= CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE,
            VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID,
            WDcfg.InterruptNumEvents, INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
                printf( "\nError configuring interrupts. Interrupts disabled\n\n");
                WDcfg.InterruptNumEvents = 0;
        }
    }
    ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, WDcfg.NumEvents);
    ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
    ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, WDcfg.ExtTriggerMode);

    if ((BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) || (BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE)){
        ret |= CAEN_DGTZ_SetGroupEnableMask(handle, WDcfg.EnableMask);
        for(i=0; i<(WDcfg.Nch/8); i++) {
            if (WDcfg.EnableMask & (1<<i)) {
                if (BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
                    for(j=0; j<8; j++) {
                        if (WDcfg.DCoffsetGrpCh[i][j] != -1)
                            ret |= CAEN_DGTZ_SetChannelDCOffset(handle,(i*8)+j, WDcfg.DCoffsetGrpCh[i][j]);
                        else
                            ret |= CAEN_DGTZ_SetChannelDCOffset(handle,(i*8)+j, WDcfg.DCoffset[i]);
                    }
                }
                else {
                    ret |= CAEN_DGTZ_SetGroupDCOffset(handle, i, WDcfg.DCoffset[i]);
                    ret |= CAEN_DGTZ_SetGroupSelfTrigger(handle, WDcfg.ChannelTriggerMode[i], (1<<i));
                    ret |= CAEN_DGTZ_SetGroupTriggerThreshold(handle, i, WDcfg.Threshold[i]);
                    ret |= CAEN_DGTZ_SetChannelGroupMask(handle, i, WDcfg.GroupTrgEnableMask[i]);
                }
                ret |= CAEN_DGTZ_SetTriggerPolarity(handle, i, WDcfg.TriggerEdge);
            }
        }
    } else {
        ret |= CAEN_DGTZ_SetChannelEnableMask(handle, WDcfg.EnableMask);
        for (i = 0; i < WDcfg.Nch; i++) {
            if (WDcfg.EnableMask & (1<<i)) {
                ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, WDcfg.DCoffset[i]);
                if (BoardInfo.FamilyCode != CAEN_DGTZ_XX730_FAMILY_CODE)
                    ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, WDcfg.ChannelTriggerMode[i], (1<<i));
                ret |= CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, WDcfg.Threshold[i]);
                ret |= CAEN_DGTZ_SetTriggerPolarity(handle, i, WDcfg.TriggerEdge);
            }
        }
        if (BoardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE) {
            // channel pair settings for x730 boards
            for (i = 0; i < WDcfg.Nch; i += 2) {
                if (WDcfg.EnableMask & (0x3 << i)) {
                    CAEN_DGTZ_TriggerMode_t mode = WDcfg.ChannelTriggerMode[i];
                    uint32_t pair_chmask = 0;
                    // Build mode and relevant channelmask. The behaviour is that,
                    // if the triggermode of one channel of the pair is DISABLED,
                    // this channel doesn't take part to the trigger generation.
                    // Otherwise, if both are different from DISABLED, the one of
                    // the even channel is used.
                    if (WDcfg.ChannelTriggerMode[i] != CAEN_DGTZ_TRGMODE_DISABLED) {
                        if (WDcfg.ChannelTriggerMode[i + 1] == CAEN_DGTZ_TRGMODE_DISABLED)
                            pair_chmask = (0x1 << i);
                        else
                            pair_chmask = (0x3 << i);
                    }
                    else {
                        mode = WDcfg.ChannelTriggerMode[i + 1];
                        pair_chmask = (0x2 << i);
                    }
                    pair_chmask &= WDcfg.EnableMask;
                    ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, mode, pair_chmask);
                }
            }
        }
    }
    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
        for(i=0; i<(WDcfg.Nch/8); i++) {
            ret |= CAEN_DGTZ_SetDRS4SamplingFrequency(handle, WDcfg.DRS4Frequency);
            ret |= CAEN_DGTZ_SetGroupFastTriggerDCOffset(handle,i,WDcfg.FTDCoffset[i]);
            ret |= CAEN_DGTZ_SetGroupFastTriggerThreshold(handle,i,WDcfg.FTThreshold[i]);
        }
    }

    /* execute generic write commands */
    for(i=0; i<WDcfg.GWn; i++)
        ret |= WriteRegisterBitmask(handle, WDcfg.GWaddr[i], WDcfg.GWdata[i], WDcfg.GWmask[i]);

    if (ret)
        printf("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n");
    else {
        printf("Sucessfully programed digitizer !\n");
    }
    return 0;
}

int daq_device_caen_v1730::ProgramDigitizer(int handle, DigitizerParams_t Params, CAEN_DGTZ_DPP_PSD_Params_t DPPParams,char* filename)
{

  /* This function uses the CAENDigitizer API functions to perform the digitizer's initial configuration */
  int i, ret = 0;

  /* Reset the digitizer already reset at the constructor */
  //ret |= CAEN_DGTZ_Reset(handle);

  if (ret) {
    printf("ERROR: can't reset the digitizer.\n");
    return -1;
  }

  ret |= CAEN_DGTZ_SetDPPAcquisitionMode(handle, Params.AcqMode, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);


  // configure interrupts and trigger
   //ret |= CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE,
   //                                          VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID,
   //                                          1, INTERRUPT_MODE);

  // Set the digitizer acquisition mode (CAEN_DGTZ_SW_CONTROLLED or CAEN_DGTZ_S_IN_CONTROLLED)
  ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);

  //Set Run syncroniztion mode
  //ret |=CAEN_DGTZ_SetRunSynchronizationMode(handle,CAEN_DGTZ_RUN_SYNC_TrgOutTrgInDaisyChain);


  // Set the I/O level (CAEN_DGTZ_IOLevel_NIM or CAEN_DGTZ_IOLevel_TTL)
  ret |= CAEN_DGTZ_SetIOLevel(handle, Params.IOlev);

  // Set the digitizer's behaviour when an external trigger arrives:

  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle,Params.ExtTriggerInputMode);

  // Set the enabled channels
  ret |= CAEN_DGTZ_SetChannelEnableMask(handle, Params.ChannelMask);

  // Set how many events to accumulate in the board memory before being available for readout
  ret |= CAEN_DGTZ_SetDPPEventAggregation(handle, Params.EventAggr, 0);

  /* Set the mode used to syncronize the acquisition between different boards.
     In this example the sync is disabled */
  ret |= CAEN_DGTZ_SetRunSynchronizationMode(handle, CAEN_DGTZ_RUN_SYNC_Disabled);

  // Set the DPP specific parameters for the channels in the given channelMask
  ret |= CAEN_DGTZ_SetDPPParameters(handle, Params.ChannelMask, &DPPParams);

  // Set the number of samples for each waveform
  ret |= CAEN_DGTZ_SetRecordLength(handle, Params.RecordLength);
  //ret |= CAEN_DGTZ_SetRecordLength(handle, 3000,1);
  //ret |= CAEN_DGTZ_SetDPPTriggerMode(handle,CAEN_DGTZ_DPP_TriggerMode_Coincidence);



  for(i=0; i<MAX_N_CHANNEL; i++) {
    if (Params.ChannelMask & (1<<i)) {
      // Set a DC offset to the input signal to adapt it to digitizer's dynamic range
      ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, Params.DCOffset[i]);
      // Set the Pre-Trigger size (in samples)
      ret |= CAEN_DGTZ_SetDPPPreTriggerSize(handle, i, Params.PreTriggerSize[i]);
      // Set the polarity for the given channel
      ret |= CAEN_DGTZ_SetChannelPulsePolarity(handle, i, Params.PulsePolarity[i]);
    }
  }
  /*
  cout<<"board"<<b<<endl;
  for (i=0;i<MAX_N_CHANNEL;i++){
    uint32_t presize = 0;
    ret |= CAEN_DGTZ_GetDPPPreTriggerSize(handle, i, &presize);
    cout<<"ch"<<i<<" -presize="<<presize<<endl;
  }
  */


  //Write Register
  FILE *infile;
  infile=fopen(filename,"r");
  char line[1000];
  uint32_t address,data,mask;
  while (fgets(line,1000,infile)!=NULL){
    char ident[500];
    char var1[500];
    char var2[500];
    char var3[500];
    sscanf(line,"%s %s %s %s",ident,var1,var2,var3);
    if (strcmp(ident,"GENERIC_WRITE")==0){
      char *p;
      address=strtoul(var1, &p, 16);
      data=strtoul(var2, &p, 16);
      mask=strtoul(var3, &p, 16);
      ret|=WriteRegisterBitmask(handle,address,data,mask);
    }
  }

  if (ret) {
    printf("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n");
    return ret;
  } else {
    return 0;
  }
}


bool daq_device_caen_v1730::checkpulse(uint16_t* pulse,int nsample, int nsbl, double threshold, CAEN_DGTZ_PulsePolarity_t pulsepol){
    bool ret=false;
    double baseline=-9999.;
    nsbl=(int) pow(2,nsbl+1);

    int totbl=0;
    for (int i=0;i<nsample;i++){
        if (i<nsbl){
            totbl+=(int) pulse[i];
            if(i==nsbl-1) {
                baseline = (double)totbl/(double)nsbl;
            }
        }else{//checking it
            if (pulsepol==CAEN_DGTZ_PulsePolarityNegative) {
                if((double)pulse[i]<baseline-threshold){
                    ret=true;
                    break;
                }
            }else{
                if((double)pulse[i]>baseline+threshold){
                    ret=true;
                    break;
                }
            }
        }
    }
    return ret;
}

bool daq_device_caen_v1730::checkpositivepulse1740(uint16_t* pulse,int nsample, int threshold, int thresholdlow){
    bool ret=false;
    for (int i=0;i<nsample;i++){
        if(pulse[i]>threshold){
            ret=true;
            break;
        }
        if (pulse[i]<thresholdlow){
            ret=true;
            break;
        }
    }
    return ret;
}
