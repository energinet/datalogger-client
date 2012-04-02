#!/bin/bash

if [ -f /sys/class/liab/modem/power ] ; then
    #modem driver is allready running 
    echo "modem driver is running"
    if [ `cat /sys/class/liab/modem/power` -eq 1 ] ; then 
	#modem is allready running
	echo "modem is running"
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
	    exit 0
	fi
	
	echo "Chat script fault"
	/etc/ifscript/modem_down.sh
	    
    fi
fi

modprobe modem

TIMEOUT=10
while  [ ! -f /sys/class/liab/modem/power ] ; do
    TIMEOUT=$((TIMEOUT-1))
    if [ $TIMEOUT -lt 0 ]; then
        exit 1
    fi
    sleep 1
done

echo "1"> /sys/class/liab/modem/power

TIMEOUT=10
while [ `cat /sys/class/liab/modem/power` -eq 0 ] ; do 
    TIMEOUT=$((TIMEOUT-1))
    if [ $TIMEOUT -lt 0 ]; then
        exit 1
    fi
    sleep 1
done

echo "1"> /sys/class/liab/modem/ignite

TIMEOUT=10
while [ `cat /sys/class/liab/modem/ignite` -eq 0 ] ; do 
    TIMEOUT=$((TIMEOUT-1))
    if [ $TIMEOUT -lt 0 ]; then
        exit 1
    fi
    sleep 1
done