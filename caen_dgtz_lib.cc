#include "caen_dgtz_lib.h"

//!* *************************************************************************************** */
//!* V1730 lib
//!* *************************************************************************************** */

int dc_file[MAX_CH];
float dc_8file[8];
int thr_file[MAX_CH] = { 0 };

/* Error messages */
static char ErrMsg[ERR_DUMMY_LAST][100] = {
    "No Error",                                         /* ERR_NONE */
    "Configuration File not found",                     /* ERR_CONF_FILE_NOT_FOUND */
    "Can't open the digitizer",                         /* ERR_DGZ_OPEN */
    "Can't read the Board Info",                        /* ERR_BOARD_INFO_READ */
    "Can't run WaveDump for this digitizer",            /* ERR_INVALID_BOARD_TYPE */
    "Can't program the digitizer",                      /* ERR_DGZ_PROGRAM */
    "Can't allocate the memory for the readout buffer", /* ERR_MALLOC */
    "Restarting Error",                                 /* ERR_RESTART */
    "Interrupt Error",                                  /* ERR_INTERRUPT */
    "Readout Error",                                    /* ERR_READOUT */
    "Event Build Error",                                /* ERR_EVENT_BUILD */
    "Can't allocate the memory fro the histograms",     /* ERR_HISTO_MALLOC */
    "Unhandled board type",                             /* ERR_UNHANDLED_BOARD */
    "Output file write error",                          /* ERR_OUTFILE_WRITE */
        "Over Temperature",									/* ERR_OVERTEMP */
        "Board Failure",									/* ERR_BOARD_FAILURE */

};


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

static CAEN_DGTZ_IRQMode_t INTERRUPT_MODE = CAEN_DGTZ_IRQ_MODE_ROAK;

/* ###########################################################################
*  Functions
*  ########################################################################### */
/*! \fn      static long get_time()
*   \brief   Get time in milliseconds
*
*   \return  time in msec
*/
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
//!###################



static void SetDefaultConfiguration(WaveDumpConfig_t *WDcfg) {
    int i, j;

    WDcfg->RecordLength = (1024*16);
        WDcfg->PostTrigger = 50;
        WDcfg->NumEvents = 1023;
        WDcfg->EnableMask = 0xFFFF;
        WDcfg->GWn = 0;
    WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
    WDcfg->InterruptNumEvents = 0;
    WDcfg->TestPattern = 0;
        WDcfg->DecimationFactor = 1;
    WDcfg->DesMode = CAEN_DGTZ_DISABLE;
        WDcfg->FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
    WDcfg->FastTriggerEnabled = 0;
        WDcfg->FPIOtype = CAEN_DGTZ_IOLevel_NIM;

        strcpy(WDcfg->GnuPlotPath, GNUPLOT_DEFAULT_PATH);
        for(i=0; i<MAX_SET; i++) {
                WDcfg->PulsePolarity[i] = CAEN_DGTZ_PulsePolarityPositive;
                WDcfg->Version_used[i] = 0;
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
    WDcfg->StartupCalibration = 1;
}
int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg) {
    char str[1000], str1[1000], *pread;
    int i, ch=-1, val, Off=0, tr = -1;
    int ret = 0;

    // Save previous values (for compare)
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

        // Acquisition Frequency (X742 only)
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
            read = fscanf(f_ini, "%d", (int*)&WDcfg->DecimationFactor);
            continue;
        }

        // Trigger Edge
        /*if (strstr(str, "TRIGGER_EDGE")!=NULL) {
                    read = fscanf(f_ini, "%s", str1);
                    if (strcmp(str1, "FALLING")==0)
                            WDcfg->TriggerEdge = 1;
                    else if (strcmp(str1, "RISING")!=0)
                            printf("%s: invalid option\n", str);
                    continue;
            }*/

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
                WDcfg->DesMode = CAEN_DGTZ_ENABLE;
            else if (strcmp(str1, "NO")!=0)
                printf("%s: invalid option\n", str);
            if(PrevDesMode != WDcfg->DesMode)
                ret |= (0x1 << CFGRELOAD_DESMODE_BIT);
            continue;
        }

        // Output file format (BINARY or ASCII)
        if (strstr(str, "OUTPUT_FILE_FORMAT")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "BINARY")==0){
                int temp=(int)WDcfg->OutFileFlags;
                temp|=(int)OFF_BINARY;
                WDcfg->OutFileFlags=(OUTFILE_FLAGS)temp;
            }
            else if (strcmp(str1, "ASCII")!=0)
                printf("%s: invalid output file format\n", str1);
            continue;
        }

        // Header into output file (YES or NO)
        if (strstr(str, "OUTPUT_FILE_HEADER")!=NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0){
                int temp=(int)WDcfg->OutFileFlags;
                temp|=(int)OFF_HEADER;
                WDcfg->OutFileFlags=(OUTFILE_FLAGS)temp;
            }

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
        ///Input polarity
        if (strstr(str, "PULSE_POLARITY")!=NULL) {
            CAEN_DGTZ_PulsePolarity_t pp= CAEN_DGTZ_PulsePolarityPositive;
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "POSITIVE") == 0)
                pp = CAEN_DGTZ_PulsePolarityPositive;
            else if (strcmp(str1, "NEGATIVE") == 0)
                pp = CAEN_DGTZ_PulsePolarityNegative;
            else
                printf("%s: Invalid Parameter\n", str);

            for (i = 0; i<MAX_SET; i++)///polarity setting (old trigger edge)is the same for all channels
                WDcfg->PulsePolarity[i] = pp;

            continue;
        }

        //DC offset (percent of the dynamic range, -50 to 50)
        if (!strcmp(str, "DC_OFFSET"))
        {

            int dc;
            read = fscanf(f_ini, "%d", &dc);
            if (tr != -1) {
                // 				WDcfg->FTDCoffset[tr] = dc;
                WDcfg->FTDCoffset[tr * 2] = (uint32_t)dc;
                WDcfg->FTDCoffset[tr * 2 + 1] = (uint32_t)dc;
                continue;
            }

            val = (int)((dc + 50) * 65535 / 100);
            if (ch == -1)
                for (i = 0; i < MAX_SET; i++)
                {
                    WDcfg->DCoffset[i] = val;
                    WDcfg->Version_used[i] = 0;
                    dc_file[i] = dc;
                }
            else
            {
                WDcfg->DCoffset[ch] = val;
                WDcfg->Version_used[ch] = 0;
                dc_file[ch] = dc;
            }

            continue;
        }


        if (!strcmp(str, "BASELINE_LEVEL"))
        {

            int dc;
            read = fscanf(f_ini, "%d", &dc);
            if (tr != -1) {
                // 				WDcfg->FTDCoffset[tr] = dc;
                WDcfg->FTDCoffset[tr * 2] = (uint32_t)dc;
                WDcfg->FTDCoffset[tr * 2 + 1] = (uint32_t)dc;
                continue;
            }

            if (ch == -1)
            {
                for (i = 0; i < MAX_SET; i++)
                {
                    WDcfg->Version_used[i] = 1;
                    dc_file[i] = dc;
                    if (WDcfg->PulsePolarity[i] == CAEN_DGTZ_PulsePolarityPositive)
                        WDcfg->DCoffset[i] = (uint32_t)((float)(fabs(dc - 100))*(655.35));

                    else if (WDcfg->PulsePolarity[i] == CAEN_DGTZ_PulsePolarityNegative)
                        WDcfg->DCoffset[i] = (uint32_t)((float)(dc)*(655.35));
                }
            }
            else
            {
                WDcfg->Version_used[ch] = 1;
                dc_file[ch] = dc;
                if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityPositive)
                {
                    WDcfg->DCoffset[ch] = (uint32_t)((float)(fabs(dc - 100))*(655.35));
                    //printf("ch %d positive, offset %d\n",ch, WDcfg->DCoffset[ch]);
                }


                else if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityNegative)
                {
                    WDcfg->DCoffset[ch] = (uint32_t)((float)(dc)*(655.35));
                    //printf("ch %d negative, offset %d\n",ch, WDcfg->DCoffset[ch]);
                }


            }

            continue;
        }

        if (strstr(str, "GRP_CH_DC_OFFSET")!=NULL) ///xx742
        {
            float dc[8];
            read = fscanf(f_ini, "%f,%f,%f,%f,%f,%f,%f,%f", &dc[0], &dc[1], &dc[2], &dc[3], &dc[4], &dc[5], &dc[6], &dc[7]);
            int j = 0;
            for( j=0;j<8;j++) dc_8file[j] = dc[j];

            for(i=0; i<8; i++) //MAX_SET
            {
                val = (int)((dc[i]+50) * 65535 / 100); ///DC offset (percent of the dynamic range, -50 to 50)
                WDcfg->DCoffsetGrpCh[ch][i]=val;
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
                for (i = 0; i < MAX_SET; i++)
                {
                    WDcfg->Threshold[i] = val;
                    thr_file[i] = val;
                }
            else
            {
                WDcfg->Threshold[ch] = val;
                thr_file[ch] = val;
            }
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
            if (strcmp(str1, "DISABLED") == 0)
                tm = CAEN_DGTZ_TRGMODE_DISABLED;
            else if (strcmp(str1, "ACQUISITION_ONLY") == 0)
                tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
            else if (strcmp(str1, "ACQUISITION_AND_TRGOUT") == 0)
                tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
            else if (strcmp(str1, "TRGOUT_ONLY") == 0)
                tm = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
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
                {
                    WDcfg->EnableMask |= (1 << ch);
                }
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

        // Skip startup calibration
        if (strstr(str, "SKIP_STARTUP_CALIBRATION") != NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES") == 0)
                WDcfg->StartupCalibration = 0;
            else
                WDcfg->StartupCalibration = 1;
            continue;
        }

        printf("%s: invalid setting\n", str);
    }
    return ret;

}
void Load_DAC_Calibration_From_Flash(int handle, WaveDumpConfig_t *WDcfg,CAEN_DGTZ_BoardInfo_t BoardInfo) {
    FLASH_API_ERROR_CODES err = FLASH_API_SUCCESS;
    uint8_t *buffer;
    int ch = 0;
    float calibration_data[2 * MAX_SET];

    err = SPIFlash_init(handle);// init flash
    if (err != FLASH_API_SUCCESS) {
            printf("Error in flash init\n");
            return;
    }

    buffer = (uint8_t*)malloc(1 + VIRTUAL_PAGE_SIZE * sizeof(uint8_t));
    memset(buffer, 0, 1 + VIRTUAL_PAGE_SIZE * sizeof(uint8_t));

    err = SPIFlash_read_virtual_page(handle, OFFSET_CALIBRATION_VIRTUAL_PAGE, buffer);
    if (err != FLASH_API_SUCCESS) {
            printf("Error reading flash page size\n");
            return;
    }
    if (buffer[0] != 0xD) {
            printf("\nNo DAC Calibration data found in board flash. Use option 'D' to calibrate DAC.\n\n");
            free(buffer);
            return;
    }
    else {
            memcpy(calibration_data, buffer + 1, 2 * MAX_SET * sizeof(float));
            for (ch = 0; ch < (int)BoardInfo.Channels; ch++) {
                    WDcfg->DAC_Calib.cal[ch] = calibration_data[2 * ch];
                    WDcfg->DAC_Calib.offset[ch] = calibration_data[1 + 2 * ch];
            }
    }

    free(buffer);
    printf("\nDAC calibration correctly loaded from board flash.\n");
}
void Save_DAC_Calibration_To_Flash(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo) {
    FLASH_API_ERROR_CODES err = FLASH_API_SUCCESS;
    uint8_t *buffer;
    int ch = 0;
    float calibration_data[2*MAX_SET];

    for (ch = 0; ch < (int)BoardInfo.Channels; ch++) {
            calibration_data[2*ch] = WDcfg.DAC_Calib.cal[ch];
            calibration_data[1 + 2 * ch] = WDcfg.DAC_Calib.offset[ch];
    }

    err = SPIFlash_init(handle);// init flash
    if (err != FLASH_API_SUCCESS) {
            printf("Error in flash init.\n");
            return;
    }

    buffer = (uint8_t*)malloc(1 + VIRTUAL_PAGE_SIZE * sizeof(uint8_t));
    memset(buffer, 0, 1 + VIRTUAL_PAGE_SIZE * sizeof(uint8_t));

    buffer[0] = 0xD;
    memcpy((buffer +1), calibration_data, VIRTUAL_PAGE_SIZE * sizeof(uint8_t));//copy cal vector to buffer

    err = SPIFlash_write_virtual_page(handle, OFFSET_CALIBRATION_VIRTUAL_PAGE,buffer);
    if (err != FLASH_API_SUCCESS) {
            printf("Error writing flash page\n");
            return;
    }

    free(buffer);

    printf("DAC calibration correctly saved on flash.\n");
}

//!* *************************************************************************************** */

void Set_relative_Threshold(int handle, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo){
    int ch = 0, i = 0;

    //preliminary check: if baseline shift is not enabled for any channel quit
    int should_start = 0;
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
            if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
                    should_start = 1;
                    break;
            }
    }
    if (!should_start)
            return;

    CAEN_DGTZ_ErrorCode ret;
    uint32_t  AllocatedSize;
    ERROR_CODES ErrCode = ERR_NONE;
    uint32_t BufferSize;
    CAEN_DGTZ_EventInfo_t       EventInfo;
    char *buffer = NULL;
    char *EventPtr = NULL;
    CAEN_DGTZ_UINT16_EVENT_t    *Event16 = NULL;
    CAEN_DGTZ_UINT8_EVENT_t     *Event8 = NULL;
    uint32_t custom_posttrg = 50, dco, custom_thr;
    float expected_baseline;
    float dco_percent;
    int baseline[MAX_CH] = { 0 }, size = 0, samples = 0;
    int no_self_triggered_event[MAX_CH] = {0};
    int sw_trigger_needed = 0;
    int event_ch;

    ///malloc
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
    if (ret) {
            ErrCode = ERR_MALLOC;
            goto QuitProgram;
    }
    if (WDcfg->Nbit == 8)
            ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event8);
    else {
            ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);
    }
    if (ret != CAEN_DGTZ_Success) {
            ErrCode = ERR_MALLOC;
            goto QuitProgram;
    }

    //some custom settings
    ret = CAEN_DGTZ_SetPostTriggerSize(handle, custom_posttrg);
    if (ret) {
            printf("Threshold calc failed. Error trying to set post trigger!!\n");
            return;
    }
    //try to set a small threshold to get a self triggered event
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
            if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
                    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
                            ret = CAEN_DGTZ_GetGroupDCOffset(handle, ch, &dco);
                    else
                            ret = CAEN_DGTZ_GetChannelDCOffset(handle, ch, &dco);
                    if (ret) {
                            printf("Threshold calc failed. Error trying to get DCoffset values!!\n");
                            return;
                    }
                    dco_percent = (float)dco / 65535.;
                    expected_baseline = pow(2, (double)BoardInfo.ADC_NBits) * (1.0 - dco_percent);

                    custom_thr = (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityPositive) ? ((uint32_t)expected_baseline + 100) : ((uint32_t)expected_baseline - 100);

                    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
                            ret = CAEN_DGTZ_SetGroupTriggerThreshold(handle, ch, custom_thr);
                    else
                            ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, ch, custom_thr);
                    if (ret) {
                            printf("Threshold calc failed. Error trying to set custom threshold value!!\n");
                            return;
                    }
            }
    }

    CAEN_DGTZ_SWStartAcquisition(handle);
#ifdef _WIN32
    Sleep(300);
#else
    usleep(300000);
#endif

    ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
    if (ret) {
            ErrCode = ERR_READOUT;
            goto QuitProgram;
    }
    //we have some self-triggered event
    if (BufferSize > 0) {
            ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, 0, &EventInfo, &EventPtr);
            if (ret) {
                    ErrCode = ERR_EVENT_BUILD;
                    goto QuitProgram;
            }
            // decode the event //
            if (WDcfg->Nbit == 8)
                    ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event8);
            else
                    ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16);

            if (ret) {
                    ErrCode = ERR_EVENT_BUILD;
                    goto QuitProgram;
            }

            for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
                    if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
                            event_ch = (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) ? (ch * 8) : ch;//for x740 boards shift to channel 0 of next group
                            size = (WDcfg->Nbit == 8) ? Event8->ChSize[event_ch] : Event16->ChSize[event_ch];
                            if (size == 0) {//no data from channel ch
                                    no_self_triggered_event[ch] = 1;
                                    sw_trigger_needed = 1;
                                    continue;
                            }
                            else {//use only one tenth of the pre-trigger samples to calculate the baseline
                                    samples = (int)(size * ((100 - custom_posttrg) / 2) / 100);

                                    for (i = 0; i < samples; i++) //mean over some pre trigger samples
                                    {
                                            if (WDcfg->Nbit == 8)
                                                    baseline[ch] += (int)(Event8->DataChannel[event_ch][i]);
                                            else
                                                    baseline[ch] += (int)(Event16->DataChannel[event_ch][i]);
                                    }
                                    baseline[ch] = (baseline[ch] / samples);
                            }

                            if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityPositive)
                                    WDcfg->Threshold[ch] = (uint32_t)baseline[ch] + thr_file[ch];
                            else 	if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityNegative)
                                    WDcfg->Threshold[ch] = (uint32_t)baseline[ch] - thr_file[ch];

                            if (WDcfg->Threshold[ch] < 0) WDcfg->Threshold[ch] = 0;
                            size = (int)pow(2, (double)BoardInfo.ADC_NBits);
                            if (WDcfg->Threshold[ch] > (uint32_t)size) WDcfg->Threshold[ch] = size;

                            if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
                                    ret = CAEN_DGTZ_SetGroupTriggerThreshold(handle, ch, WDcfg->Threshold[ch]);
                            else
                                    ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, ch, WDcfg->Threshold[ch]);
                            if (ret)
                                    printf("Warning: error setting ch %d corrected threshold\n", ch);
                    }
            }
    }
    else {
            sw_trigger_needed = 1;
            for(ch = 0; ch < (int32_t)BoardInfo.Channels; ch++)
                    no_self_triggered_event[ch] = 1;
    }

    CAEN_DGTZ_ClearData(handle);

    //if from some channels we had no self triggered event, we send a software trigger
    if (sw_trigger_needed) {
            CAEN_DGTZ_SendSWtrigger(handle);

            ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
            if (ret) {
                    ErrCode = ERR_READOUT;
                    goto QuitProgram;
            }
            if (BufferSize == 0)
                    return;

            ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, 0, &EventInfo, &EventPtr);
            if (ret) {
                    ErrCode = ERR_EVENT_BUILD;
                    goto QuitProgram;
            }
            // decode the event //
            if (WDcfg->Nbit == 8)
                    ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event8);
            else
                    ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16);

            if (ret) {
                    ErrCode = ERR_EVENT_BUILD;
                    goto QuitProgram;
            }

            for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
                    if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
                            event_ch = (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) ? (ch * 8) : ch;//for x740 boards shift to channel 0 of next group
                            size = (WDcfg->Nbit == 8) ? Event8->ChSize[event_ch] : Event16->ChSize[event_ch];
                            if (!no_self_triggered_event[ch])//we already have a good baseline for channel ch
                                    continue;

                            //use some samples to calculate the baseline
                            for (i = 1; i < 11; i++){ //mean over 10 samples
                                    if (WDcfg->Nbit == 8)
                                            baseline[ch] += (int)(Event8->DataChannel[event_ch][i]);
                                    else
                                            baseline[ch] += (int)(Event16->DataChannel[event_ch][i]);
                            }
                                    baseline[ch] = (baseline[ch] / 10);
                    }

                    if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityPositive)
                            WDcfg->Threshold[ch] = (uint32_t)baseline[ch] + thr_file[ch];
                    else 	if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityNegative)
                            WDcfg->Threshold[ch] = (uint32_t)baseline[ch] - thr_file[ch];

                    if (WDcfg->Threshold[ch] < 0) WDcfg->Threshold[ch] = 0;
                            size = (int)pow(2, (double)BoardInfo.ADC_NBits);
                    if (WDcfg->Threshold[ch] > (uint32_t)size) WDcfg->Threshold[ch] = size;

                    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
                            ret = CAEN_DGTZ_SetGroupTriggerThreshold(handle, ch, WDcfg->Threshold[ch]);
                    else
                            ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, ch, WDcfg->Threshold[ch]);
                    if (ret)
                                    printf("Warning: error setting ch %d corrected threshold\n", ch);
            }
    }//end sw trigger event analysis

    CAEN_DGTZ_SWStopAcquisition(handle);

    //reset posttrigger
    ret = CAEN_DGTZ_SetPostTriggerSize(handle, WDcfg->PostTrigger);
    if (ret)
            printf("Error resetting post trigger.\n");

    CAEN_DGTZ_ClearData(handle);

    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    if (WDcfg->Nbit == 8)
            CAEN_DGTZ_FreeEvent(handle, (void**)&Event8);
    else
            CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);


QuitProgram:
    if (ErrCode) {
            printf("\a%s\n", ErrMsg[ErrCode]);
#ifdef WIN32
            printf("Press a key to quit\n");
            getch();
#endif
    }
}

void Calibrate_DC_Offset(int handle, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo){
    float cal[MAX_CH];
    float offset[MAX_CH] = { 0 };
    int i = 0, k = 0, p = 0, acq = 0, ch = 0;
    for (i = 0; i < MAX_CH; i++)
            cal[i] = 1;
    long ret;
    CAEN_DGTZ_AcqMode_t mem_mode;
    uint32_t  AllocatedSize;

    ERROR_CODES ErrCode = ERR_NONE;
    uint32_t BufferSize;
    CAEN_DGTZ_EventInfo_t       EventInfo;
    char *buffer = NULL;
    char *EventPtr = NULL;
    CAEN_DGTZ_UINT16_EVENT_t    *Event16 = NULL;
    CAEN_DGTZ_UINT8_EVENT_t     *Event8 = NULL;

    float avg_value[NPOINTS][MAX_CH] = { 0 };
    uint32_t dc[NPOINTS] = { 25,75 }; //test values (%)
    uint32_t chmask = 0;

    ret = CAEN_DGTZ_GetAcquisitionMode(handle, &mem_mode);//chosen value stored
    if (ret)
            printf("Error trying to read acq mode!!\n");
    ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
    if (ret)
            printf("Error trying to set acq mode!!\n");
    ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
    if (ret)
            printf("Error trying to set ext trigger!!\n");
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++)
            chmask |= (1 << ch);
    ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, chmask);
    if (ret)
            printf("Warning: error disabling channels self trigger\n");
    ret = CAEN_DGTZ_SetChannelEnableMask(handle, chmask);
    if (ret)
            printf("Warning: error enabling channels.\n");
    ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle, 1);
    if (ret)
            printf("Warning: error setting max BLT number\n");
    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE || BoardInfo.FamilyCode == CAEN_DGTZ_XX724_FAMILY_CODE) {
            ret = CAEN_DGTZ_SetDecimationFactor(handle, 1);
            if (ret)
                    printf("Error trying to set decimation factor!!\n");
    }

    ///malloc
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
    if (ret) {
            ErrCode = ERR_MALLOC;
            goto QuitProgram;
    }
    if (WDcfg->Nbit == 8)
            ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event8);
    else {
            ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);
    }
    if (ret != CAEN_DGTZ_Success) {
            ErrCode = ERR_MALLOC;
            goto QuitProgram;
    }

    printf("Starting DAC calibration...\n");

    for (p = 0; p < NPOINTS; p++){
            //set new dco  test value to all channels
            for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
                            ret = CAEN_DGTZ_SetChannelDCOffset(handle, (uint32_t)ch, (uint32_t)((float)(abs(dc[p] - 100))*(655.35)));
                            if (ret)
                                    printf("Error setting ch %d test offset\n", ch);
            }
#ifdef _WIN32
                                    Sleep(200);
#else
                                    usleep(200000);
#endif
            CAEN_DGTZ_ClearData(handle);

            ret = CAEN_DGTZ_SWStartAcquisition(handle);
            if (ret){
                    printf("Error starting acquisition\n");
                    goto QuitProgram;
            }

            int value[NACQS][MAX_CH] = { 0 };//baseline values of the NACQS
            for (acq = 0; acq < NACQS; acq++){
                    CAEN_DGTZ_SendSWtrigger(handle);

                    ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
                    if (ret) {
                                    ErrCode = ERR_READOUT;
                                    goto QuitProgram;
                    }
                    if (BufferSize == 0)
                            continue;
                    ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, 0, &EventInfo, &EventPtr);
                    if (ret) {
                                    ErrCode = ERR_EVENT_BUILD;
                                    goto QuitProgram;
                    }
                    // decode the event //
                    if (WDcfg->Nbit == 8)
                            ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event8);
                            else
                            ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16);

                    if (ret) {
                            ErrCode = ERR_EVENT_BUILD;
                            goto QuitProgram;
                    }

                    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++){
                                    for (i = 1; i < 21; i++) //mean over 20 samples
                                    {
                                            if (WDcfg->Nbit == 8)
                                                    value[acq][ch] += (int)(Event8->DataChannel[ch][i]);
                                            else
                                                    value[acq][ch] += (int)(Event16->DataChannel[ch][i]);
                                    }
                                    value[acq][ch] = (value[acq][ch] / 20);
                    }

            }//for acq

            ///check for clean baselines
            for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
                            int max = 0, ok = 0;
                            int mpp = 0;
                            int size = (int)pow(2, (double)BoardInfo.ADC_NBits);
                            int *freq = (int*) calloc(size, sizeof(int));

                            //find most probable value mpp
                            for (k = 0; k < NACQS; k++) {
                                    if (value[k][ch] > 0 && value[k][ch] < size) {
                                            freq[value[k][ch]]++;
                                            if (freq[value[k][ch]] > max) {
                                                    max = freq[value[k][ch]];
                                                    mpp = value[k][ch];
                                            }
                                    }
                            }
                            free(freq);
                            //discard values too far from mpp
                            for (k = 0; k < NACQS; k++) {
                                    if (value[k][ch] >= (mpp - 5) && value[k][ch] <= (mpp + 5)) {
                                            avg_value[p][ch] = avg_value[p][ch] + (float)value[k][ch];
                                            ok++;
                                    }
                            }
                            //calculate final best average value
                            avg_value[p][ch] = (avg_value[p][ch] / (float)ok)*100. / (float)size;
            }

            CAEN_DGTZ_SWStopAcquisition(handle);
    }//close for p

    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
                    cal[ch] = ((float)(avg_value[1][ch] - avg_value[0][ch]) / (float)(dc[1] - dc[0]));
                    offset[ch] = (float)(dc[1] * avg_value[0][ch] - dc[0] * avg_value[1][ch]) / (float)(dc[1] - dc[0]);
                    printf("Channel %d DAC calibration ready.\n", ch);
                    //printf("Channel %d --> Cal %f   offset %f\n", ch, cal[ch], offset[ch]);

                    WDcfg->DAC_Calib.cal[ch] = cal[ch];
                    WDcfg->DAC_Calib.offset[ch] = offset[ch];
    }

    CAEN_DGTZ_ClearData(handle);

     ///free events e buffer
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    if (WDcfg->Nbit == 8)
            CAEN_DGTZ_FreeEvent(handle, (void**)&Event8);
    else
            CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);

    //reset settings
    ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, WDcfg->NumEvents);
    ret |= CAEN_DGTZ_SetPostTriggerSize(handle, WDcfg->PostTrigger);
    ret |= CAEN_DGTZ_SetAcquisitionMode(handle, mem_mode);
    ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, WDcfg->ExtTriggerMode);
    ret |= CAEN_DGTZ_SetChannelEnableMask(handle, WDcfg->EnableMask);
    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE || BoardInfo.FamilyCode == CAEN_DGTZ_XX724_FAMILY_CODE)
            ret |= CAEN_DGTZ_SetDecimationFactor(handle, WDcfg->DecimationFactor);
    if (ret)
            printf("Error resetting some parameters after DAC calibration\n");

    //reset self trigger mode settings
    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE || BoardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE) {
            // channel pair settings for x730 boards
            for (i = 0; i < WDcfg->Nch; i += 2) {
                    if (WDcfg->EnableMask & (0x3 << i)) {
                            CAEN_DGTZ_TriggerMode_t mode = WDcfg->ChannelTriggerMode[i];
                            uint32_t pair_chmask = 0;

                            if (WDcfg->ChannelTriggerMode[i] != CAEN_DGTZ_TRGMODE_DISABLED) {
                                    if (WDcfg->ChannelTriggerMode[i + 1] == CAEN_DGTZ_TRGMODE_DISABLED)
                                            pair_chmask = (0x1 << i);
                                    else
                                            pair_chmask = (0x3 << i);
                            }
                            else {
                                    mode = WDcfg->ChannelTriggerMode[i + 1];
                                    pair_chmask = (0x2 << i);
                            }

                            pair_chmask &= WDcfg->EnableMask;
                            ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, mode, pair_chmask);
                    }
            }
    }
    else {
            for (i = 0; i < WDcfg->Nch; i++) {
                    if (WDcfg->EnableMask & (1 << i))
                            ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, WDcfg->ChannelTriggerMode[i], (1 << i));
            }
    }
    if (ret)
            printf("Error resetting self trigger mode after DAC calibration\n");

    Save_DAC_Calibration_To_Flash(handle, *WDcfg, BoardInfo);

QuitProgram:
    if (ErrCode) {
            printf("\a%s\n", ErrMsg[ErrCode]);
#ifdef WIN32
            printf("Press a key to quit\n");
            getch();
#endif
    }

}

void Calibrate_XX740_DC_Offset(int handle, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo){
    float cal[MAX_CH];
    float offset[MAX_CH] = { 0 };
    int i = 0, acq = 0, k = 0, p=0, g = 0;
    for (i = 0; i < MAX_CH; i++)
            cal[i] = 1;
    long ret;
    CAEN_DGTZ_AcqMode_t mem_mode;
    uint32_t  AllocatedSize;

    ERROR_CODES ErrCode = ERR_NONE;
    uint32_t BufferSize;
    CAEN_DGTZ_EventInfo_t       EventInfo;
    char *buffer = NULL;
    char *EventPtr = NULL;
    CAEN_DGTZ_UINT16_EVENT_t    *Event16 = NULL;

    float avg_value[NPOINTS][MAX_CH] = { 0 };
    uint32_t dc[NPOINTS] = { 25,75 }; //test values (%)
    uint32_t groupmask = 0;

    ret = CAEN_DGTZ_GetAcquisitionMode(handle, &mem_mode);//chosen value stored
    if (ret)
            printf("Error trying to read acq mode!!\n");
    ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
    if (ret)
            printf("Error trying to set acq mode!!\n");
    ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
    if (ret)
            printf("Error trying to set ext trigger!!\n");
    ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle, 1);
    if (ret)
            printf("Warning: error setting max BLT number\n");
    ret = CAEN_DGTZ_SetDecimationFactor(handle, 1);
    if (ret)
            printf("Error trying to set decimation factor!!\n");
    for (g = 0; g< (int32_t)BoardInfo.Channels; g++) //BoardInfo.Channels is number of groups for x740 boards
            groupmask |= (1 << g);
    ret = CAEN_DGTZ_SetGroupSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, groupmask);
    if (ret)
            printf("Error disabling self trigger\n");
    ret = CAEN_DGTZ_SetGroupEnableMask(handle, groupmask);
    if (ret)
            printf("Error enabling channel groups.\n");
    ///malloc
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
    if (ret) {
            ErrCode = ERR_MALLOC;
            goto QuitProgram;
    }

    ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);
    if (ret != CAEN_DGTZ_Success) {
            ErrCode = ERR_MALLOC;
            goto QuitProgram;
    }

    printf("Starting DAC calibration...\n");

    for (p = 0; p < NPOINTS; p++){
            for (i = 0; i < (int32_t)BoardInfo.Channels; i++) { //BoardInfo.Channels is number of groups for x740 boards
                            ret = CAEN_DGTZ_SetGroupDCOffset(handle, (uint32_t)i, (uint32_t)((float)(abs(dc[p] - 100))*(655.35)));
                            if (ret)
                                    printf("Error setting group %d test offset\n", i);
            }
    #ifdef _WIN32
                            Sleep(200);
    #else
                            usleep(200000);
    #endif

    CAEN_DGTZ_ClearData(handle);

    ret = CAEN_DGTZ_SWStartAcquisition(handle);
    if (ret) {
            printf("Error starting X740 acquisition\n");
            goto QuitProgram;
    }

    int value[NACQS][MAX_CH] = { 0 }; //baseline values of the NACQS
    for (acq = 0; acq < NACQS; acq++) {
            CAEN_DGTZ_SendSWtrigger(handle);

            ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
            if (ret) {
                    ErrCode = ERR_READOUT;
                    goto QuitProgram;
            }
            if (BufferSize == 0)
                    continue;
            ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, 0, &EventInfo, &EventPtr);
            if (ret) {
                    ErrCode = ERR_EVENT_BUILD;
                    goto QuitProgram;
            }
            // decode the event //
            ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16);
            if (ret) {
                    ErrCode = ERR_EVENT_BUILD;
                    goto QuitProgram;
            }
            for (g = 0; g < (int32_t)BoardInfo.Channels; g++) {
                            for (k = 1; k < 21; k++) //mean over 20 samples
                                    value[acq][g] += (int)(Event16->DataChannel[g * 8][k]);

                            value[acq][g] = (value[acq][g] / 20);
            }

    }//for acq

    ///check for clean baselines
    for (g = 0; g < (int32_t)BoardInfo.Channels; g++) {
                    int max = 0;
                    int mpp = 0;
                    int size = (int)pow(2, (double)BoardInfo.ADC_NBits);
                    int *freq = (int*) calloc(size, sizeof(int));
                    //find the most probable value mpp
                    for (k = 0; k < NACQS; k++) {
                            if (value[k][g] > 0 && value[k][g] < size) {
                                    freq[value[k][g]]++;
                                    if (freq[value[k][g]] > max) {
                                            max = freq[value[k][g]];
                                            mpp = value[k][g];
                                    }
                            }
                    }
                    free(freq);
                    //discard values too far from mpp
                    int ok = 0;
                    for (k = 0; k < NACQS; k++) {
                            if (value[k][g] >= (mpp - 5) && value[k][g] <= (mpp + 5)) {
                                    avg_value[p][g] = avg_value[p][g] + (float)value[k][g];
                                    ok++;
                            }
                    }
                    avg_value[p][g] = (avg_value[p][g] / (float)ok)*100. / (float)size;
    }

    CAEN_DGTZ_SWStopAcquisition(handle);
    }//close for p

    for (g = 0; g < (int32_t)BoardInfo.Channels; g++) {
                    cal[g] = ((float)(avg_value[1][g] - avg_value[0][g]) / (float)(dc[1] - dc[0]));
                    offset[g] = (float)(dc[1] * avg_value[0][g] - dc[0] * avg_value[1][g]) / (float)(dc[1] - dc[0]);
                    printf("Group %d DAC calibration ready.\n",g);
                    printf("Cal %f   offset %f\n", cal[g], offset[g]);

                    WDcfg->DAC_Calib.cal[g] = cal[g];
                    WDcfg->DAC_Calib.offset[g] = offset[g];
    }

    CAEN_DGTZ_ClearData(handle);

    ///free events e buffer
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);

    CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);

    ret |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, WDcfg->NumEvents);
    ret |= CAEN_DGTZ_SetDecimationFactor(handle,WDcfg->DecimationFactor);
    ret |= CAEN_DGTZ_SetPostTriggerSize(handle, WDcfg->PostTrigger);
    ret |= CAEN_DGTZ_SetAcquisitionMode(handle, mem_mode);
    ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, WDcfg->ExtTriggerMode);
    ret |= CAEN_DGTZ_SetGroupEnableMask(handle, WDcfg->EnableMask);
    for (i = 0; i < BoardInfo.Channels; i++) {
            if (WDcfg->EnableMask & (1 << i))
                    ret |= CAEN_DGTZ_SetGroupSelfTrigger(handle, WDcfg->ChannelTriggerMode[i], (1 << i));
    }
    if (ret)
            printf("Error setting recorded parameters\n");

    Save_DAC_Calibration_To_Flash(handle, *WDcfg, BoardInfo);

QuitProgram:
            if (ErrCode) {
                    printf("\a%s\n", ErrMsg[ErrCode]);
#ifdef WIN32
                    printf("Press a key to quit\n");
                    getch();
#endif
            }
}

int Set_calibrated_DCO(int handle, int ch, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo) {
    int ret = CAEN_DGTZ_Success;
    if (WDcfg->Version_used[ch] == 0) //old DC_OFFSET config, skip calibration
            return ret;
    if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityPositive) {
            WDcfg->DCoffset[ch] = (uint32_t)((float)(fabs((((float)dc_file[ch] - WDcfg->DAC_Calib.offset[ch]) / WDcfg->DAC_Calib.cal[ch]) - 100.))*(655.35));
            if (WDcfg->DCoffset[ch] > 65535) WDcfg->DCoffset[ch] = 65535;
            if (WDcfg->DCoffset[ch] < 0) WDcfg->DCoffset[ch] = 0;
    }
    else if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_PulsePolarityNegative) {
            WDcfg->DCoffset[ch] = (uint32_t)((float)(fabs(((fabs(dc_file[ch] - 100.) - WDcfg->DAC_Calib.offset[ch]) / WDcfg->DAC_Calib.cal[ch]) - 100.))*(655.35));
            if (WDcfg->DCoffset[ch] < 0) WDcfg->DCoffset[ch] = 0;
            if (WDcfg->DCoffset[ch] > 65535) WDcfg->DCoffset[ch] = 65535;
    }

    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) {
            ret = CAEN_DGTZ_SetGroupDCOffset(handle, (uint32_t)ch, WDcfg->DCoffset[ch]);
            if (ret)
                    printf("Error setting group %d offset\n", ch);
    }
    else {
            ret = CAEN_DGTZ_SetChannelDCOffset(handle, (uint32_t)ch, WDcfg->DCoffset[ch]);
            if (ret)
                    printf("Error setting channel %d offset\n", ch);
    }

    return ret;
}



int GetMoreBoardInfo(int handle, CAEN_DGTZ_BoardInfo_t BoardInfo, WaveDumpConfig_t *WDcfg){
    int ret;
    switch(BoardInfo.FamilyCode) {
        CAEN_DGTZ_DRS4Frequency_t freq;

    case CAEN_DGTZ_XX724_FAMILY_CODE:
    case CAEN_DGTZ_XX781_FAMILY_CODE:
    case CAEN_DGTZ_XX782_FAMILY_CODE:
    case CAEN_DGTZ_XX780_FAMILY_CODE:
        WDcfg->Nbit = 14; WDcfg->Ts = 10.0; break;
    case CAEN_DGTZ_XX720_FAMILY_CODE: WDcfg->Nbit = 12; WDcfg->Ts = 4.0;  break;
    case CAEN_DGTZ_XX721_FAMILY_CODE: WDcfg->Nbit =  8; WDcfg->Ts = 2.0;  break;
    case CAEN_DGTZ_XX731_FAMILY_CODE: WDcfg->Nbit =  8; WDcfg->Ts = 2.0;  break;
    case CAEN_DGTZ_XX751_FAMILY_CODE: WDcfg->Nbit = 10; WDcfg->Ts = 1.0;  break;
    case CAEN_DGTZ_XX761_FAMILY_CODE: WDcfg->Nbit = 10; WDcfg->Ts = 0.25;  break;
    case CAEN_DGTZ_XX740_FAMILY_CODE: WDcfg->Nbit = 12; WDcfg->Ts = 16.0; break;
    case CAEN_DGTZ_XX725_FAMILY_CODE: WDcfg->Nbit = 14; WDcfg->Ts = 4.0; break;
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
                case CAEN_DGTZ_DRS4_750MHz:
            WDcfg->Ts = (float)(1.0/750.0)*1000.0;
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
    case CAEN_DGTZ_XX782_FAMILY_CODE:
    case CAEN_DGTZ_XX780_FAMILY_CODE:
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
    case CAEN_DGTZ_XX725_FAMILY_CODE:
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

int WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask) {
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
int CheckBoardFailureStatus(int handle, CAEN_DGTZ_BoardInfo_t BoardInfo) {

    int ret = 0;
    uint32_t status = 0;
    ret = CAEN_DGTZ_ReadRegister(handle, 0x8104, &status);
    if (ret != 0) {
            printf("Error: Unable to read board failure status.\n");
            return -1;
    }
#ifdef _WIN32
    Sleep(200);
#else
    usleep(200000);
#endif
    //read twice (first read clears the previous status)
    ret = CAEN_DGTZ_ReadRegister(handle, 0x8104, &status);
    if (ret != 0) {
            printf("Error: Unable to read board failure status.\n");
            return -1;
    }

    if(!(status & (1 << 7))) {
            printf("Board error detected: PLL not locked.\n");
            return -1;
    }

    return 0;
}
void GoToNextEnabledGroup(WaveDumpRun_t *WDrun, WaveDumpConfig_t *WDcfg) {
    if ((WDcfg->EnableMask) && (WDcfg->Nch>8)) {
        int orgPlotIndex = WDrun->GroupPlotIndex;
        do {
            WDrun->GroupPlotIndex = (++WDrun->GroupPlotIndex)%(WDcfg->Nch/8);
        } while( !((1 << WDrun->GroupPlotIndex)& WDcfg->EnableMask));
        if( WDrun->GroupPlotIndex != orgPlotIndex) {
            printf("Plot group set to %d\n", WDrun->GroupPlotIndex);
        }
    }
    //ClearPlot();
}
int32_t BoardSupportsCalibration(CAEN_DGTZ_BoardInfo_t BoardInfo) {
    return
                BoardInfo.FamilyCode == CAEN_DGTZ_XX761_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE;
}
int32_t BoardSupportsTemperatureRead(CAEN_DGTZ_BoardInfo_t BoardInfo) {
    return
        BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE;
}
void calibrate(int handle, WaveDumpRun_t *WDrun, CAEN_DGTZ_BoardInfo_t BoardInfo) {
    printf("\n");
    if (BoardSupportsCalibration(BoardInfo)) {
        if (WDrun->AcqRun == 0) {
            int32_t ret = CAEN_DGTZ_Calibrate(handle);
            if (ret == CAEN_DGTZ_Success) {
                printf("ADC Calibration check: the board is calibrated.\n");
            }
            else {
                printf("ADC Calibration failed. CAENDigitizer ERR %d\n", ret);
            }
            printf("\n");
        }
        else {
            printf("Can't run ADC calibration while acquisition is running.\n");
        }
    }
    else {
        printf("ADC Calibration not needed for this board family.\n");
    }
}

int ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo){
    int i, j, ret = 0;

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
        ret |= CAEN_DGTZ_SetFastTriggerDigitizing(handle,(CAEN_DGTZ_EnaDis_t) WDcfg.FastTriggerEnabled);
        ret |= CAEN_DGTZ_SetFastTriggerMode(handle,WDcfg.FastTriggerMode);
    }
    if ((BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) || (BoardInfo.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE)) {
        ret |= CAEN_DGTZ_SetDESMode(handle, WDcfg.DesMode);
    }
    ret |= CAEN_DGTZ_SetRecordLength(handle, WDcfg.RecordLength);
    ret |= CAEN_DGTZ_GetRecordLength(handle, &WDcfg.RecordLength);

    if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE || BoardInfo.FamilyCode == CAEN_DGTZ_XX724_FAMILY_CODE) {
        ret |= CAEN_DGTZ_SetDecimationFactor(handle, WDcfg.DecimationFactor);
    }

    ret |= CAEN_DGTZ_SetPostTriggerSize(handle, WDcfg.PostTrigger);
    if(BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE) {
        uint32_t pt;
        ret |= CAEN_DGTZ_GetPostTriggerSize(handle, &pt);
        WDcfg.PostTrigger = pt;
    }
    ret |= CAEN_DGTZ_SetIOLevel(handle, WDcfg.FPIOtype);
    if( WDcfg.InterruptNumEvents > 0) {
        // Interrupt handling
        if( ret |= CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE,
            VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID,
            (uint16_t)WDcfg.InterruptNumEvents, INTERRUPT_MODE)!= CAEN_DGTZ_Success) {
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
                            ret |= CAEN_DGTZ_SetChannelDCOffset(handle, (i * 8) + j, WDcfg.DCoffset[i]);

                    }
                }
                else {
                    if(WDcfg.Version_used[i] == 1)
                                                ret |= Set_calibrated_DCO(handle, i, &WDcfg, BoardInfo);
                                        else
                                                ret |= CAEN_DGTZ_SetGroupDCOffset(handle, i, WDcfg.DCoffset[i]);
                    ret |= CAEN_DGTZ_SetGroupSelfTrigger(handle, WDcfg.ChannelTriggerMode[i], (1<<i));
                    ret |= CAEN_DGTZ_SetGroupTriggerThreshold(handle, i, WDcfg.Threshold[i]);
                    ret |= CAEN_DGTZ_SetChannelGroupMask(handle, i, WDcfg.GroupTrgEnableMask[i]);
                }
                ret |= CAEN_DGTZ_SetTriggerPolarity(handle, i, (CAEN_DGTZ_TriggerPolarity_t) WDcfg.PulsePolarity[i]); //.TriggerEdge

            }
        }
    } else {
        ret |= CAEN_DGTZ_SetChannelEnableMask(handle, WDcfg.EnableMask);
        for (i = 0; i < WDcfg.Nch; i++) {
            if (WDcfg.EnableMask & (1<<i)) {
                                if (WDcfg.Version_used[i] == 1)
                                        ret |= Set_calibrated_DCO(handle, i, &WDcfg, BoardInfo);
                                else
                                        ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, WDcfg.DCoffset[i]);
                if (BoardInfo.FamilyCode != CAEN_DGTZ_XX730_FAMILY_CODE &&
                    BoardInfo.FamilyCode != CAEN_DGTZ_XX725_FAMILY_CODE)
                    ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle, WDcfg.ChannelTriggerMode[i], (1<<i));
                ret |= CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, WDcfg.Threshold[i]);
                ret |= CAEN_DGTZ_SetTriggerPolarity(handle, i, (CAEN_DGTZ_TriggerPolarity_t) WDcfg.PulsePolarity[i]); //.TriggerEdge
            }
        }
        if (BoardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE ||
            BoardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE) {
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

    return 0;
}
