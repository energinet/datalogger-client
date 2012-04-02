#!/bin/bash -x
logger -t "watchdog" -i -s "Watchdog check" 2> /dev/console 
# create a lockfile so we are not run multiple times
if [ ! -e /tmp/watchdog-check-lock ]; then
  : > /tmp/watchdog-check-lock
else
    logger -t "watchdog" -i -s "Is locked" 2> /dev/console 
    exit 0
fi

# delete log files so that disk won't run full
if [ -d /var/log/watchdog ]; then
    rm -f /var/log/watchdog/test-bin.*
fi

if [ -f /jffs2/config ]; then
    source /jffs2/config
fi

if ps aux | grep "[/]bin/sh /root/tunnel"; then
    rm -f /tmp/watchdog-check-lock
    logger -t "watchdog" -i -s "Exit 'tunnel' is running" 2> /dev/console 
    exit 0
fi


CONFIG=/tmp/config
DOWNCNTFILE=/tmp/downcnt

if [ -f $CONFIG ]; then
    STATE=`grep STATE_CONNECTION $CONFIG | cut -d= -f2`
    case $STATE in
        3g)
            PPTP_TUNNEL=1
            ;;
        ethernet)
            PPTP_TUNNEL=1
            ;;
    esac

    if [ $PPTP_TUNNEL -eq 1 ]; then

        if [ ! -e $DOWNCNTFILE ]; then
            echo "0" > $DOWNCNTFILE
            DOWNCNT=0
        else
            DOWNCNT=`cat $DOWNCNTFILE`
        fi

        logger -t "watchdog" -i -s "Run ping dc" $DOWNCNT 2> /dev/console 
        # Is the tunnel interface running?
        ADDR=172.16.0.1
        if ! ping -w 12 -c 4 $ADDR > /jffs2/ping.log ; then
            
            DOWNCNT=`expr $DOWNCNT + 1`
            echo $DOWNCNT > $DOWNCNTFILE
            logger -t "watchdog" -i -s "Tunnel interface down." $DOWNCNT 2> /dev/console 
           


            if [ $DOWNCNT -gt 4 ] ; then
            # stop the existing tunnel
                /root/tunnel stop
                
                case $STATE in
                    3g)
                        if [ -f /root/3g ]; then
                            if [ ! -e /tmp/restart-3g ]; then
                                logger -t "watchdog" -i -s "Stopping 3G. Restarting 3G on next call" 2> /dev/console 
                                : > /tmp/restart-3g
                                /root/3g stop
                                killall -9 pppd
                            else
                                logger -t "watchdog" -i -s " Restarting 3G" 2> /dev/console 
                                rm -f /tmp/restart-3g
                                /root/3g start
                                sleep 2
                                /root/tunnel start
                            fi
                        fi
                        ;;
                esac
            fi
        else
            echo "0" > $DOWNCNTFILE
        fi
        logger -t "watchdog" -i -s "ping:" `cat /jffs2/ping.log | grep transmitted` 2> /dev/console  
        logger -t "watchdog" -i -s "ping:" `cat /jffs2/ping.log | grep rtt` 2> /dev/console 
    fi  
fi

rm -f /var/log/watchdog/*
rm -f /tmp/watchdog-check-lock
logger -t "watchdog" -i -s "Exit" 2> /dev/console 