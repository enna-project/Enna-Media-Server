#! /bin/sh
# -------------------
# Enna Media Server
# -------------------
# CDROM util called by udev when a CDROM is inserted or removed.
# /usr/sbin/ems_cdrom_changed.sh
# First argument is the device name (eg. /dev/sr0)
# Second argument is "inserted" or "removed".
# ----------------------------------------------------------------------

if [ "$#" != "2" ]; then
	echo "Usage: $0 device {inserted|removed}"
	exit 1
fi

device="${1}"
action="${2}"

if [ "${action}" = "inserted" ]; then
	# Make sure MPD can read/ioctl the device
	chmod 777 ${device}

	# Warn EMS
	dbus-send --system --type=signal /com/EnnaMediaServer/Cdrom com.EnnaMediaServer.Cdrom.inserted string:"${device}"

	# Configure the CDROM
	if which hdparm &> /dev/null; then
		# Disable power management
		hdparm -B 255 ${device}
		# Avoid unwanted spin down
		hdparm -S 10 ${device}
		# Set speed to x10 to reduce noise, it is enough to read
		# CDDA without underruns
		hdparm -E 10 ${device}
	fi

	exit 0
fi

if [ "${action}" = "removed" ]; then
	# Warn EMS
	dbus-send --system --type=signal /com/EnnaMediaServer/Cdrom com.EnnaMediaServer.Cdrom.removed string:"${device}"
	exit 0
fi

echo "Unrecognized action, should be either 'inserted' or 'removed'"

exit 1
