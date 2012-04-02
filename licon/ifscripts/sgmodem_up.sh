#!/bin/bash


function find_text() {
    TO=$3
    RV=1

    until [[ $RV -ne 1  || $TO -lt 0 ]]; do
        grep -qi "$2" $1
        RV=$?
        sleep 1
        TO=$((TO-1))
        echo -n "."
    done
    if [ $TO -lt 0 ]; then
        # Fail
        return 1
    else
        # Success
        return 0
    fi
}

function test_attty() {
    TTYDEV=$1
    if chat -t 30 -T "$PPPAPN" -E					\
	    TIMEOUT  	5				\
	    ECHO 		  ON				\
	    ABORT		  '\nBUSY\r'			\
	    ABORT		  '\nERROR\r'			\
	    ABORT		  '\nNO ANSWER\r'			\
	    ABORT		  '\nNO CARRIER\r'		\
	    ABORT		  '\nNO DIALTONE\r'		\
	    ABORT     '\nRINGING\r\n\r\nRINGING\r'	\
	    ''	   	  \rAT				\
	    TIMEOUT	  5				\
	    SAY             "Testing AT interface"   \
	    OK		    "" \
	    SAY		    "\nOK..."  0</dev/ttyS2 1>/dev/ttyS2 ; then
	    echo "Chat script success"
	    return 1
    fi
    
    return 0
    

}


if [ `cat /sys/class/gpio/gpio41/value` -eq 1 ] ; then
    #modem is running
    if ! find_text /proc/bus/usb/devices "Manufacturer=Cinterion"  5 ; then  
	TTYDEV="/dev/ttyS2"
    else
	TTYDEV="/dev/ttyACM0"
    fi
    
    if test_attty $TTYDEV ; then 
	exit 0
    fi

fi

modprobe cdc-acm

ls /dev/ttyACM0
if [ $? -eq 2 ] ; then
    echo "Creating node for ttyACM0"
    mknod /dev/ttyACM0 c 166 0
fi

echo "Powering on modem"


echo 1 > /sys/class/gpio/gpio76/value
sleep 0.2
echo 0 > /sys/class/gpio/gpio77/value
sleep 0.3



TIMEOUT=10
while  [ `cat /sys/class/gpio/gpio41/value` -eq 0 ] ; do
    TIMEOUT=$((TIMEOUT-1))
    if [ $TIMEOUT -lt 0 ]; then
        echo "Power on error"
        exit 1
    fi
    sleep 1
done

echo "Powered up"


if ! find_text /proc/bus/usb/devices "Manufacturer=Cinterion"  5 ; then  
    TTYDEV="/dev/ttyS2"
    echo "GPRS modem"
else
    TIMEOUT=10
    while [ ! -x /sys/class/tty/ttyACM0/ ] ; do 
	TIMEOUT=$((TIMEOUT-1))
	if [ $TIMEOUT -lt 0 ]; then
            echo "Could not find modem in /sys/class/tty/ttyACM0/"
            exit 1
	fi
	echo -n "."
	sleep 1
    done
    echo "EGPRS modem"
    TTYDEV="/dev/ttyACM0"
fi

echo tty de is $TTYDEV

# Reset modem 
chat -E					\
	TIMEOUT  	1				\
	ECHO 		 ON				\
	ABORT		  '\nBUSY\r'			\
	ABORT		  '\nERROR\r'			\
	ABORT		  '\nNO ANSWER\r'			\
	ABORT		  '\nNO CARRIER\r'		\
	ABORT		  '\nNO DIALTONE\r'		\
	ABORT     '\nRINGING\r\n\r\nRINGING\r'	\
	'' ATZ \
	OK '' 0> $TTYDEV 1< $TTYDEV

exit 0