AUTOMAKE_OPTIONS = foreign


AM_LDFLAGS = -L$(libdir)


INCLUDES = -I$(includedir) $(WXINCLUDES) $(RPC_INCLUDE)


include_HEADERS = 

lib_LTLIBRARIES =  librcdaqplugin_CAENdrs.la

librcdaqplugin_CAENdrs_la_SOURCES = \
	daq_device_CAENdrs_std.cc \
	daq_device_CAENdrs.cc \
	CAENdrsTriggerHandler.cc \
	CAENdrs_plugin.cc


librcdaqplugin_CAENdrs_la_LIBADD = -lpthread -lCAENDigitizer $(RPC_LDD)
bin_PROGRAMS =  caen_client
caen_client_SOURCES = caen_client.cc parseargument.cc caen_manager.cc

caen_client_LDADD = -lpthread -lCAENDigitizer


clean-local:
	rm -f $(BUILT_SOURCES)
