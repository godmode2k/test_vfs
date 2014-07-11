#!/bin/sh

#
# VFS Driver Project Build Script"
#
# Author: Ho-Jung Kim (godmode2k@hotmail.com)
# Date: Since February 21, 2013
# Last modified: March 4, 2013
#



BIN_CC=/usr/bin/gcc

OPT_INC_PATH=""
OPT_LIB_PATH=""
OPT_ALL="-Wall -O2	\
	-D__LINUX__	\
	-D__REQ_MAIN_FUNCTION__	\
	$SRC_IN_LIB_PATH_ALL $OPT_INC_PATH $OPT_LIB_PATH $OPT_LIBS	\
"

SRC_OUT=$1
SRC_IN=$2

BUILD_CMD="$BIN_CC -o $SRC_OUT $SRC_IN $SRC_IN_LIB $OPT_ALL"

if [ -z "$1" -a -z "$2" ]; then
	echo $0
	echo "Usage: sh $0 [output] [input]"
	echo ""
	exit
fi
if [ -z "$2" ]; then
	echo $0
	echo "Usage: sh $0 [output] [input]"
	echo ""
	exit
fi



echo "-------------------------------"
echo "VFS Driver Project Build Script"
echo "-------------------------------"
echo "[+] build... [START]"
echo $BUILD_CMD
$BIN_CC -o $SRC_OUT $SRC_IN $SRC_IN_LIB $OPT_ALL

#echo ""
# Checks the last command return value
if [ $? -ne 0 ]; then
	echo ""
	echo "[-] build... [FINISHED] [FAIL]"
else
	echo ""
	echo "[+] build... [FINISHED] [SUCCESS]"
fi

# EOF
