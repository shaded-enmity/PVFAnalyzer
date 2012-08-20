#!/bin/bash

source $BASEDIR/testing/scripts/global.sh.inc
if [ $? != 0 ]; then
	exit 1;
fi

$ROOTDIR/build/reader/reader -f $TESTCASES/cfgbuilding/recursion.bin -o $TESTOUT/recursion.cfg -e 0x1d
$ROOTDIR/build/printer/printer -f $TESTOUT/recursion.cfg -o -
