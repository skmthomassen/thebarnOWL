#!/bin/bash

if [ $# -ne 1 ] ; then
    echo "Give fucking arg.."
    exit 10
fi

ARG=$1
TODAY=$(date +"%Y-%m-%d")
DAYANDTIME=$(date +"%Y-%m-%d_%H:%M:%S")
JUSTTIME=$(date +"%H:%M:%S")
#DSTAMP="2018-03-16"
#TSTAMP="14:25:09"
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

TODAY_DIR=$SCRIPT_DIR/clips/$TODAY
mkdir -p $TODAY_DIR

#echo $DSTAMP
#echo $TSTAMP
echo "TODAYDIR: " $TODAY_DIR

#camera network addresses
URLA='rtsp://root:hest1234@192.168.130.203/axis-media/media.amp'
URLB='rtsp://root:hest1234@192.168.130.204/axis-media/media.amp'
RTSPSRC_SETTINGS='ntp-sync=true protocols=GST_RTSP_LOWER_TRANS_TCP ntp-time-source=1'
CAPS_SETTINGS='application/x-rtp,media=video'

case $ARG in
  -2)
  GST_DEBUG=3 gst-launch-1.0 -ev \
    mpegtsmux name=mux \
    ! filesink location=$TODAY_DIR/$JUSTTIME.ts \
    rtspsrc location=$URLA $RTSPSRC_SETTINGS \
    ! queue ! capsfilter caps=$CAPS_SETTINGS ! rtph264depay \
    ! mux. \
    rtspsrc location=$URLB $RTSPSRC_SETTINGS \
    ! queue ! capsfilter caps=$CAPS_SETTINGS ! rtph264depay \
    ! mux. \
  ;;
  -1)
  GST_DEBUG=3 gst-launch-1.0 -ev \
    multifilesrc location=$TODAY_DIR/"12:11:09"_%05d.mkv \
    ! rtph264depay ! h264parse \
    ! matroskamux \
    ! filesink location=$TODAY_DIR/"12:11:09"_complete.mkv
  ;;
esac

#
#







exit 0
