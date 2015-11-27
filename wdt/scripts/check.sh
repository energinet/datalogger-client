#!/bin/bash -x

LOGGER="logger -t 'watchdog' -p"

UPTIME=`cat /proc/uptime |  cut -d '.' -f1`
RESETFILE=/tmp/wdtreset
RETVAL=0

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

#========== Make sure not to check for these things every 20 seconds ======
if [ -f $RESETFILE ] ; then 
	RESETUPTIME=`cat $RESETFILE`
else 
	RESETUPTIME=0;
fi

RESETUPTIME=`expr 1200 + $RESETUPTIME`

if [ $UPTIME -lt $RESETUPTIME ]; then
	exit 0
else
	echo $UPTIME > $RESETFILE
fi

#============================== Start checks ==============================


#Check if the log has been written within the previous 600 seconds
if ! logdbutil -w600 ; then
    echo "Watchdog check db write failed" > /dev/console
    killall contdaem 
	RETVAL=1
    logdbutil -S "watchdog killed contdaem"
	$LOGGER warning "killed contdaem due to no updates for more than 600 seconds"
fi


if ! logdbutil -b3600 ; then # If no log is transmitted for one hour, then restart rpclient.
    echo "Watchdog check db backup failed" > /dev/console

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
    else
		echo "rpclient not running" > /dev/console
    fi
	RETVAL=2
fi

if ! logdbutil -b86400 ; then # If no log is transmitted for 24 hours, then reboot.
    echo "Watchdog check db backup failed for 24 hours" > /dev/console
	logdbutil -S "no transmitted data, requesting watchdog reboot"
	echo "Requesting reboot (uptime $UPTIME)" > /dev/console
	RETVAL=-2
fi

rm -f /var/log/watchdog/*

exit $RETVAL
