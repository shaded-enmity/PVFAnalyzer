#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/loopif.bin -o $TESTOUT/loopif.cfg -e 0x5
$ROOTDIR/build/printer/printer -f $TESTOUT/loopif.cfg -o -
