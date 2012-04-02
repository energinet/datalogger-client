#!/bin/bash 
WDTWAITFILE="/tmp/wdt_wait"
INSPECTLIST="liabconnect contdaem httpd modbusd"
echo "Watchdog repair $1" > /dev/console
logfile=/tmp/wdt_repair.log
#logfile=&2
#if [ $1 ] ; then
    retval=$1
#else
#    retval=0
#fi

#
date > $logfile



if [ -f $WDTWAITFILE ] ; then 
    WAITUNTIL=`cat $WDTWAITFILE`
    CURRENTTIME=`date +%s`
    if [ $CURRENTTIME -le $WAITUNTIL ] ; then
	echo "Dog has bone ("`expr $WAITUNTIL - $CURRENTTIME`" sec left)" >> $logfile
	if [ `expr $WAITUNTIL - $CURRENTTIME` -gt 3000 ] ; then
	    echo "But it is to big" >> $logfile
	    rm $WDTWAITFILE;
	else
	    exit 0
	fi
	
    else
	echo "Wait is maxed" >> $logfile
    fi
fi



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
restart_process()	# Check if a process of the name Arg_1 is running
#===================================================================
# Arg_1 = name
# return 1 if the process is started 
# return 0 if the process could not be started
{
    killall -9 $1 >> $logfile
    #try to restart eesd
    $1  >> $logfile
    sleep 1
    #check if update process is running
    if [ `check_process_run $1` == "running" ] ; then 
	echo "restarted"
        return
    fi

    echo "dead"
    return
}

#===================================================================
inspect_process()	# Check if a process of the name Arg_1 is running
                        # and try restarting if application is down 
#===================================================================
# Arg_1 = name
# return 1 if the process is started 
# return 0 if the process could not be started
{
    INSPECT=$1 

    echo "inspecting: $INSPECT"  >> $logfile
    #check if process is runnign 
    if [ `check_process_run $INSPECT` == "running" ] ; then
	echo "$INSPECT is running" >> $logfile
	echo "running"
	return 
    fi

    #try restarting process
    echo "$INSPECT is not running" >> $logfile
    
    if [ `restart_process $INSPECT` == "restarted" ] ; then
        echo "$INSPECT was restarted" >> $logfile
	logdbutil -S "$INSPECT restarted"
        echo "restarted"
	return 
    fi
    
    echo "error restarting $INSPECT" >> $logfile
    logdbutil -S "$INSPECT dead"
    echo "dead"
    return 
}


restarted=0

for INSPECT in $INSPECTLIST ; do
    INSPRESULT=`inspect_process $INSPECT`
    case "$INSPRESULT" in
	"dead"     )
	    echo "$INSPECT is not running" >> $logfile
	    ;;
	"restarted")
	    echo "$INSPECT is restarted" >> $logfile
	    restarted=1
	    ;;
	*          )
	    echo "$INSPECT is running" >> $logfile
	    ;;
    esac
done

if [ $restarted -eq 1 ] ; then
     retval=0
fi

# write return value
echo "returning $retval" >> $logfile

# Notyfy console in case of reboot 
if [ $retval ] ; then
    echo "Watchdog rebooting system:" > /dev/console
    cat $logfile  > /dev/console
fi

if [ `cat /proc/uptime |  cut -d '.' -f1` -lt 600 ] ; then 
    echo "uptime to low" > /dev/console
    exit 0 
fi

rm /var/log/watchdog/test-bin.*


exit $retval
