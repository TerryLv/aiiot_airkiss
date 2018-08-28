#/bin/sh
# Name:   reset_wifi_wpa.sh
# Data:   8-20-2018
# Author: Terry Lv
# Description: Used to reset wifi on wpa.

WPA_FILE="/etc/wpa_supplicant.conf"
WLAN_DEV_NAME=$1

if [ -z "${WLAN_DEV_NAME}" ]; then
	echo "Error: please specify a device name, e.g. setup_wifi_wpa.sh wlan0"
	exit 1	
fi

ifconfig ${WLAN_DEV_NAME} down

if [ -s "/var/run/wpa_supplicant/${WLAN_DEV_NAME}" ]; then
	rm /var/run/wpa_supplicant/${WLAN_DEV_NAME}
fi

echo "Stop wpa_supplicant service ..."
killall wpa_supplicant

cat << EOF > ${WPA_FILE}
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=1

network={
        key_mgmt=NONE
}
EOF
echo "WPA reset successfully!"

