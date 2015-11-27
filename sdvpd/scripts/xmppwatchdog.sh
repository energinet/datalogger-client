#!/bin/bash

LOGGER="logger -t `basename $0` -p"
TRIES=3

#Test if sdvpd is installed
[ -x /usr/bin/sdvpd ] || {
	$LOGGER error "sdvpd not present, aborting"
	exit 1
}

#Test if xmppwatchdog is installed
[ -x /usr/bin/xmppwatchdog ] || {
	$LOGGER error "xmppwatchdog not present, aborting"
	exit 1
}

#Test if sdvpd is running
[ "$(pidof sdvpd)" ] || {
	$LOGGER error "sdvpd is not running, liabconnect should be restarting it"
#remove pid file (if present) to make sure liabconnect reacts
	rm -f /var/run/sdvpd.pid
	exit 1
}

#Test if sdvpd wrote the username to /tmp/sdvpd.jid
[ -f /tmp/sdvpd.jid ] || {
	$LOGGER error "/tmp/sdvpd.jid does not exist, aborting"
	exit 1
}

USERNAME=`cat /tmp/sdvpd.jid | cut -d '@' -f 1`@sdvp.dk

$LOGGER info "Testing XMPP connectivity to $USERNAME"

#Check if we have a previous watchdog value, if not, set it to zero
if [ -f /tmp/xmppwatchdog.cnt ]; then
	PREVWDT=`cat /tmp/xmppwatchdog.cnt`
else
	PREVWDT=0
fi

while [ $TRIES -gt 0 ]; do

	WDT=$(xmppwatchdog $USERNAME)
	STATUS=$?

	if [ $STATUS -eq 0 ]; then
		if [ $WDT -ne $PREVWDT ]; then
			$LOGGER info "sdvpd running ok (watchdog counter: $WDT)"
			echo $WDT > /tmp/xmppwatchdog.cnt
			break
		else
			$LOGGER warning "sdvpd returned same watchdog counter as last check ($WDT)"
		fi
	fi

	let TRIES=TRIES-1
	if [ $TRIES -gt 0 ]; then
		$LOGGER warning "sdvpd failed to respond, testing $TRIES more time(s)"
	fi
done

if [ $TRIES -eq 0 ]; then
	$LOGGER error "sdvpd failed to respond, killing it"
	killall -9 sdvpd
	rm -rf /var/run/sdvpd.pid
fi


