#!/bin/bash -x

PROGNAME="$1"
FILENAME="$2"
UPDATEDIR="$3"
ETCDIR="/etc"
JFFCONFDIR="/jffs2"


#===================================================================
check_process_run()	# Check if a process of the name Arg_1 is running
#===================================================================
# Arg_1 = process name
# return 1 if process is running
# return 0 if process is not running
{
    INSPECT=$1

    if [ `ps aux  | grep -v grep | grep -v $0 | grep -c $INSPECT` -gt 0 ] ; then 
	echo "running"
        return
    fi

    echo "not_running"
    return

}

#===================================================================
wait_until_kill()
#===================================================================
# Arg_1 = process name
{
    PROCESS=$1
    TIMEOUT=$2

    while [ `check_process_run $PROCESS` == running ] ; do
	if [ $TIMEOUT -le 0 ] ; then 
	    echo "timeout"
	    return
	else
	    TIMEOUT=`expr $TIMEOUT - 1`
	fi	
    done
    
    echo "killed"
    return

}


#===================================================================
watchdog_set_wait()  # Set a wait interval for the watchdog
#===================================================================
# Arg_1 = wait interval
{
    WDTWAITFILE="/tmp/wdt_wait"
    WDTWAITINTV=$1
    CURRENTTIME=`date +%s`
    WDTWAITTIME=`expr $CURRENTTIME + $WDTWAITINTV`
    echo $WDTWAITTIME > $WDTWAITFILE
}

#===================================================================
watchdog_stop_wait()  # Stop the wait interval for the watchdog
#===================================================================
# Arg_1 = wait interval
{
    WDTWAITFILE="/tmp/wdt_wait"
    rm $WDTWAITFILE
}



#===================================================================
#===================================================================
# Update script
#===================================================================
#===================================================================
# Give a bone to the watchdog 
watchdog_set_wait 300

# Stop program (soft)
killall -2 $PROGNAME 

if [ `wait_until_kill $PROGNAME 15` == "timeout" ] ; then
    # Stop program (hard)
    killall -9 $PROGNAME 
    sleep 2
fi

# copy new config for test and start program
cp "$UPDATEDIR"/"$FILENAME" "$ETCDIR"/"$FILENAME"

$PROGNAME 

sleep 10
ps aux

#check if program has is running
if [ `check_process_run $PROGNAME` == "not_running" ] ; then 
    # if not revert
    cp "$JFFCONFDIR"/"$FILENAME" "$ETCDIR"/"$FILENAME"
    $PROGNAME 
    watchdog_stop_wait
    exit 1
fi
    
cp "$UPDATEDIR"/"$FILENAME" "$JFFCONFDIR"/"$FILENAME"

watchdog_stop_wait

exit 0


