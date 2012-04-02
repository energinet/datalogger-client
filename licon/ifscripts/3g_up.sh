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






modprobe usb-storage
modprobe sd_mod
modprobe usbserial vendor=0x12d1 product=0x1001

if ! grep "HUAWEI Mobile" /proc/bus/usb/devices  ; then
    exit 2
fi






usb_modeswitch -c /etc/usb_modeswitch.conf

if ! find_text /proc/tty/driver/usbserial "4: module:usbserial name:" 10 ; then
    echo "Error. No modem found. Exiting."
    exit 1
fi