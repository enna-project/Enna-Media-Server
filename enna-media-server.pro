TEMPLATE = subdirs
SUBDIRS = src/ems

OTHER_FILES = AUTHORS COPYING README.md

# INSTALL RULES
# -----------------------------
isEmpty(PREFIX) {
    PREFIX = /usr/local
}

# Udev rules
# By default, assume Udev is installed on the target
# If not, you have to set WITH_UDEV=no in the qmake cmd line
!equals(WITH_UDEV, "no") {
    udev.path = /etc/udev/rules.d
    udev.files = script/99-ems-cdrom-manager.rules
    udev.files += script/99-ems-sndcard-manager.rules
    INSTALLS += udev
}

script.path = $$PREFIX/sbin
script.files = script/ems_cdrom_changed.sh
INSTALLS += script

# Smartmon script
!equals(WITH_SMARTMON, "no") {
    smartmon.path = /etc/smartmontools/run.d
    smartmon.files = script/70smartnotifier
    INSTALLS += smartmon
}

# Database SQL file
database.path = $$PREFIX/share/ems
database.files = doc/database/database.sql
INSTALLS += database

# Systemd unit (default)
!equals(WITH_SYSTEMD, "no") {
    systemd.path = /lib/systemd/system
    systemd.files = systemd/ems.service
    INSTALLS += systemd
}
