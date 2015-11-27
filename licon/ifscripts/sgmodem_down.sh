#!/bin/bash

echo 0 > /sys/class/gpio/gpio76/value #IGN
echo 1 > /sys/class/gpio/gpio77/value #EMERG
