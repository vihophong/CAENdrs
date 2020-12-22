AUTOMAKE_OPTIONS = foreign


AM_LDFLAGS = -L$(libdir)


INCLUDES = -I$(includedir) $(WXINCLUDES) $(RPC_INCLUDE)


include_HEADERS = 

lib_LTLIBRARIES =  librcdaqplugin_CAENdrs.la

librcdaqplugin_CAENdrs_la_SOURCES = \
        spi.cc \
        flash.cc \
        fft.cc \
        X742CorrectionRoutines.cc \
        caen_dgtz_lib.cc \
        caen_lib.cc \
        daq_device_caenadc.cc \
        daq_device_caentdc.cc \
        daq_device_caen_v1730.cc \
	CAENdrsTriggerHandler.cc \
	CAENdrs_plugin.cc

librcdaqplugin_CAENdrs_la_LIBADD = -lpthread -lCAENDigitizer -lCAENVME $(RPC_LDD)

bin_PROGRAMS =  cmdvmeA32 cmdvmeA16

cmdvmeA32_SOURCES = cmdvmeA32.cc
cmdvmeA32_LDADD = -lpthread -lCAENDigitizer -lCAENVME

cmdvmeA16_SOURCES = cmdvmeA16.cc
cmdvmeA16_LDADD = -lpthread -lCAENDigitizer -lCAENVME

clean-local:
	rm -f $(BUILT_SOURCES)
