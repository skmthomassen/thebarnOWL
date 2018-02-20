#!/bin/bash

RTSPIP3='192.168.130.203'
RTSPIP4='192.168.130.204'
RTSPIP5='192.168.130.205' 
SCRIPTNAME=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPTNAME")
OUTLOCATION=$SCRIPTPATH'/img/'
IMGRIGHT=$(date +"%Y%m%d-%H%M%S")'_r'
IMGLEFT=$(date +"%Y%m%d-%H%M%S")'_l'
IMGFORMAT='.png'

echo "IMGRIGHT: " $IMGRIGHT
echo "IMGLEFT: " $IMGLEFT
echo "SCRIPTNAME: " $SCRIPTNAME
echo "SCRIPTPATH: " $SCRIPTPATH
echo "OUTLOCATION: " $OUTLOCATION

gst-launch-1.0 -qe \
	souphttpsrc user-id="root" user-pw="hest1234" \
	location=http://192.168.130.203/axis-cgi/jpg/image.cgi?camera=1 \
	! filesink location=$OUTLOCATION$IMGLEFT$IMGFORMAT

gst-launch-1.0 -qe \
	souphttpsrc user-id="root" user-pw="hest1234" \
	location=http://192.168.130.204/axis-cgi/jpg/image.cgi?camera=1 \
	! filesink location=$OUTLOCATION$IMGRIGHT$IMGFORMAT









































exit 0