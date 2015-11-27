#!/bin/bash

TTYDEV=/dev/ttyS2

function test_attty() {
    TTYDEV=$1
	chat -t 20 -E					\
		''	   	  \rAT				\
		OK		"" 					\
		0<$TTYDEV 1>$TTYDEV
	if [ $? -eq 0 ]; then
		return 0
	else
		return 1
	fi

}

killall -q pppd

echo 0 > /sys/class/gpio/gpio76/value #IGN
echo 1 > /sys/class/gpio/gpio77/value #EMERG

sleep 1

OFF=`cat /sys/class/gpio/gpio41/value`

echo 1 > /sys/class/gpio/gpio76/value
echo 0 > /sys/class/gpio/gpio77/value

TIMEOUT=10
while  [ `cat /sys/class/gpio/gpio41/value` -eq 0 ] ; do
    TIMEOUT=$((TIMEOUT-1))
    if [ $TIMEOUT -lt 0 ]; then
        echo "Power on failed"
        exit 1
    fi
	restoremodem
done

#Set the baud rate
stty -F $TTYDEV 115200

#Wait for ^SYSSTART
if [ $OFF ]; then
	chat -t 10 -E SYSSTART 0<$TTYDEV
fi

test_attty $TTYDEV
TEST=$?
if ! [[ $TEST -eq 0 ]]; then
	echo "Power on failed"
fi

exit $TEST
