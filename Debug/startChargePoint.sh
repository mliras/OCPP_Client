#!/bin/bash

#avoid conflicts with SPI kernel driver
gdb -x gdbinit ./ChargePointClient &
sleep 2

rmmod spi_bcm2835
modprobe spi_bcm2835

sleep 0.2
sudo service neard-explorenfc stop
sudo service neard-explorenfc start

