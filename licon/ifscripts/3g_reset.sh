#!/bin/bash


if [ -f /sys/bus/usb/devices/1-2/idVendor ] ; then
    if [ `cat /sys/bus/usb/devices/1-2/idVendor` == "12d1" ] ; then
        echo "0" > /sys/class/leds/usbpwr-bot/brightness
        sleep 5
        echo "1" > /sys/class/leds/usbpwr-bot/brightness
    fi
fi


if [ -f /sys/bus/usb/devices/1-1/idVendor ] ; then
    if [ `cat /sys/bus/usb/devices/1-1/idVendor` == "12d1" ] ; then
        echo "0" > /sys/class/leds/usbpwr-top/brightness
        sleep 5
        echo "1" > /sys/class/leds/usbpwr-top/brightness
    fi
fi
