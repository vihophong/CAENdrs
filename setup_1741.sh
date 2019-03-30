#! /bin/sh

# bring the unit to reasonable defaults we don't have to set everything
caen_client init

SPEED=0    # 5GS
#SPEED=1    # 2.5GS
#SPEED=0    # 1GS
caen_client SetDRS4SamplingFrequency $SPEED

# 1024 is the max, but we can set smaller sampling depths
caen_client SetRecordLength  1024

NIM=0
TTL=1
caen_client SetIOLevel $NIM


# the delay value is in % - that can be confusing.
# also note that the 1741 sets the closest match - readback can be different
# this is oterwise known as the trigger delay
POST=12
caen_client SetPostTriggerSize $POST

# set higher baseline (default 0x8f00, lower value = higher baseline)
# just one parameter means set set for all channels
#  else channel value for individual settings.
caen_client SetChannelDCOffset 0x6000
