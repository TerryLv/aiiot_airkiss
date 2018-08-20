#/bin/sh
# Name:   reset_wifi_wpa.sh
# Data:   8-20-2018
# Author: Terry Lv
# Description: Used to reset wifi on wpa.

WPA_BAK_FILE="./wpa_supplicant.conf.bak"
WPA_FILE="/etc/wpa_supplicant.conf"
WLAN_DEV_NAME=$1

if [ -z "${WLAN_DEV_NAME}" ]; then
	echo "Error: please specify a device name, e.g. setup_wifi_wpa.sh wlan0"
	exit 1	
fi

# Backup existing wpa_supplicant configuration
if [ ! -s ${WPA_BAK_FILE} ]; then
	echo "Error: WPA backup file not exists!"
	exit 1
fi

ifconfig ${WLAN_DEV_NAME} down

if [ -s "/var/run/wpa_supplicant/${WLAN_DEV_NAME}" ]; then
	rm /var/run/wpa_supplicant/${WLAN_DEV_NAME}
fi

cp -f $WPA_BAK_FILE $WPA_FILE

