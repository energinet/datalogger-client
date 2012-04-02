#!/bin/bash -x

echo 0 > /sys/class/gpio/gpio76/value
echo 1 > /sys/class/gpio/gpio77/value

#echo "0"> /sys/class/liab/modem/power

#rmmod modem