# Creative SB0540 infrared receiver Linux driver

After compiling with `make` and `make install` (the latter as root or through `sudo`),
you'll need to create a few files to load the driver correctly.

Create a `/etc/udev/load_hid_creative_sb0540.sh` file with the content:
```
#!/bin/bash
DRIVER=$1
DEVICE=$2
HID_DRV_PATH=/sys/bus/hid/drivers
/sbin/modprobe hid_creative_sb0540
echo ${DEVICE} > ${HID_DRV_PATH}/hid-generic/unbind
echo ${DEVICE} > ${HID_DRV_PATH}/hid-creative-sb0540/bind
```

Create a `/etc/udev/rules.d/80-creative-sb0540.rules` file with the content:
```
DRIVER=="hid-generic", ENV{MODALIAS}=="hid:*v0000041ep00003100", RUN+="/bin/sh /etc/udev/load_hid_creative_sb0540.sh hid-generic %k"
```

After a reboot, the driver should be loaded automatically when the dongle gets plugged in.
