#!/bin/bash +x


#echo "Watchdog check $1" > /dev/console
UPTIME=`cat /proc/uptime |  cut -d '.' -f1`
#===================================================================
check_process_run()	# Check if a process of the name Arg_1 is running
#===================================================================
# Arg_1 = process name
# return 1 if process is running
# return 0 if process is not running
{
    INSPECT=$1

    if [ `ps aux  | grep -v grep | grep -c $INSPECT` -gt 0 ] ; then 
	echo "running"
        return
    fi

    echo "not_running"
    return

}

#===================================================================
check_uptimereset()	# If uptime is to low
#===================================================================
{
    DELAY=$1
    UPTIME=`cat /proc/uptime |  cut -d '.' -f1`
    RESETFILE=/tmp/wdtreset
   
    if [ -f $RESETFILE ] ; then 
	RESETUPTIME=`cat $RESETFILE`
    else 
	RESETUPTIME=0;
    fi 

    RESETUPTIME=`expr $DELAY + $RESETUPTIME`
    
    if [ $UPTIME -lt $RESETUPTIME ] ; then 
	echo "uptime to low ($UPTIME<$RESETUPTIME): no check" > /dev/console
	exit 0 
    fi

    echo $UPTIME > $RESETFILE

    return 1;
}

if ! logdbutil -w600 ; then
    check_uptimereset 1200
    echo "Watchdog check db write failed" > /dev/console
    killall contdaem 
    logdbutil -S "watchdog killed contdaem"
fi


if ! logdbutil -b3600 ; then # If no log is transmittet for one hour, then restart rpclient.
    echo "Watchdog check db backup failed" > /dev/console
    check_uptimereset 1200

    if [ `check_process_run rpclient` == "running" ] ; then 
	killall liabconnect -2 
	echo "liabconnect interrupted" > /dev/console
	sleep 2
	poff -a
	echo "ppp off" > /dev/console
	killall udhcpd 
	killall rpclient -2
	echo "rpclient interrupted" > /dev/console
	logdbutil -S "watchdog interrupted rpclient"
	echo $UPTIME > $RESETFILE
    else
	echo "rpclient not running" > /dev/console
    fi
fi

if ! logdbutil -b86400 ; then # If no log is transmittet for three days, then reboot.
    echo "Watchdog check db backup failed for 24 hours" > /dev/console

    if [ $UPTIME -gt 86400 ] ; then 
	logdbutil -S "no transmitted data watchdog killed system "
	echo "Rebooting system (uptime $UPTIME)" > /dev/console
	sleep 10
	reboot -f
	exit 0
    fi
fi

rm /var/log/watchdog/test-bin.*

exit 0
