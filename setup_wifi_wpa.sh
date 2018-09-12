#!/bin/sh
# Name:   setup_wifi_wpa.sh
# Data:   7-24-2018
# Author: Terry Lv
# Description: Used to setup wifi on wpa.

WPA_BAK_FILE="/etc/wpa_supplicant.conf.bak"
WPA_FILE="/etc/wpa_supplicant.conf"
WLAN_DEV_NAME="$1"
WLAN_SSID="$2"
WLAN_PASSWD="$3"

echo "Stop wpa_supplicant service ..."
killall wpa_supplicant

if [ -z "${WLAN_DEV_NAME}" ]; then
	echo "Error: please specify a device name, e.g. setup_wifi_wpa.sh wlan0 test1 12345678"
	exit 1	
fi

if [ -z "${WLAN_SSID}" ]; then
	echo "Error: please specify a WLAN SSID, e.g. setup_wifi_wpa.sh wlan0 test1 12345678"
	exit 1	
fi

if [ -z "${WLAN_PASSWD}" ]; then
	echo "Error: please specify a password for WLAN, e.g. setup_wifi_wpa.sh wlan0 test1 12345678"
	exit 1	
fi

# Backup existing wpa_supplicant configuration
if [ ! -s ${WPA_BAK_FILE} ]; then
	cp  $WPA_FILE $WPA_BAK_FILE
fi

# Determine if the network credentials are already stored in wpa_supplicant.conf
ssid_exists=false 
if grep "$WLAN_SSID" ${WPA_FILE}; then
	ssid_exists=true
fi

# If credentials do not already exist, prompt the user to enter passphrase for the network
if [ $ssid_exists == "false" ]; then
	echo "Please enter the passphrase for the network, then press Enter:"
	wpa_passphrase "$WLAN_SSID" "$WLAN_PASSWD" >> ${WPA_FILE}
fi

if [ -s "/var/run/wpa_supplicant/${WLAN_DEV_NAME}" ]; then
	rm /var/run/wpa_supplicant/${WLAN_DEV_NAME}
fi

# Start wpa_supplicant and obtain IP address from AP
echo "wpa_supplicant -B -i${WLAN_DEV_NAME} -c${WPA_FILE} -Dnl80211"
wpa_supplicant -B -i${WLAN_DEV_NAME} -c${WPA_FILE} -Dnl80211
udhcpc -i ${WLAN_DEV_NAME}

