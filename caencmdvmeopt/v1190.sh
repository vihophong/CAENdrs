#!/bin/sh
if [ -f "tdcchannellist.txt" ]
then
    
cmdvme="cmdvmeA32"
opt=""
addr="0x3333"
echo "load tdc setting ..."

# set geometry
$cmdvme $opt -ww "$addr"100e 6
sleep 0

# Hardware Reset
$cmdvme $opt -ww "$addr"1014 1
sleep 0

# Software Reset
$cmdvme $opt -ww "$addr"1016 1
sleep 0

# compansation enbale(0x0020)/disable(0x0000)
# empty evt (0x0008)      : Write global header when there are no data
# event fifo ena (0x0100) : Can read word count in one event 
# BERR enable (0x0001)    : User BERR fro Block transfer
# comp. disable + empty evt + event fifo ena = 0x0108
# comp. enable + empty evt + event fifo ena = 0x0128 
# comp. enable + empty evt + event fifo ena = 0x0029 for BERR readout
#$cmdvme $opt -ww 0x00001000 0x0100
#$cmdvme $opt -ww 0x00001000 0x0128
$cmdvme $opt -ww "$addr"1000 0x0029
sleep 1

# Interrupt Level = 1  --> startup
$cmdvme $opt -ww "$addr"100a 0
sleep 0

# Almost Full Level //5 is good value
# 3 is also ok (may have firmware dependnce)
$cmdvme $opt -ww "$addr"1022 5
sleep 1


cat tdcchannellist.txt | while read CHN
do
# from here: channel dependance 0x"code"xx
# Set Triger Matching Mode
    echo "setting channel $CHN..."
    $cmdvme $opt -qn "$addr"0000 0x00"$CHN"
    sleep 0
# Set Window Width (1=25ns, 16=400ns)
# 10.5us = 10500ns = 420
# 15.5us = 15500ns = 620
# 40.5us = 15500ns = 1620
    $cmdvme $opt -qw "$addr"0000 0x10"$CHN" 40
#$cmdvme $opt -qw "$addr"0000 0x1000 128
    sleep 0
# Set Window Offset (-2048 to +40, 1=25ns, -4=-100ns)
# -10us = -10000ns = -400
# -15us = -15000ns = -600
# -40us = -15000ns = -1600
    $cmdvme $opt -qw "$addr"0000 0x11"$CHN" -20
#$cmdvme $opt -qw "$addr"0000 0x1100 -120
    sleep 0

# Set Edge Detection (1=trailing, 2=leading, 3=both)
    $cmdvme $opt -qw "$addr"0000 0x22"$CHN" 2
    sleep 0

# Set Extra Search Margin (1=25ns, default=8)
    $cmdvme $opt -qw "$addr"0000 0x12"$CHN" 8
    sleep 0

# Set Reject Margin (1=25ns, default=4)
    $cmdvme $opt -qw "$addr"0000 0x13"$CHN" 2
    sleep 0

# Enable Subtraction of Trigger Time
    $cmdvme $opt -qn "$addr"0000 0x14"$CHN"
    sleep 0

# disable TDC Header, Trailer
#$cmdvme $opt -qn "$addr"0000 0x3100
#sleep 1

# enable TDC Header, Trailer
    $cmdvme $opt -qn "$addr"0000 0x30"$CHN"
    sleep 0
    echo "finished setting channel $CHN"
done

## Module Software clear again
$cmdvme $opt -ww "$addr"1016 1
sleep 0


fi

