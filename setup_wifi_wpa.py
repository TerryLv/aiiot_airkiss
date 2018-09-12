#! /usr/bin/python
# -*- coding: utf-8 -*-

import subprocess
import os
import time

rtn_val = 1

# Backup existing wpa_supplicant configuration
if os.path.isfile("/etc/wpa_supplicant.conf.bak") == False:
	wpa_bak = open("/etc/wpa_supplicant.conf.bak", "w")
	wpa_orig = subprocess.check_output(["cat", "/etc/wpa_supplicant.conf"])
	wpa_bak.write(wpa_orig)
	wpa_bak.close()

# Prompt user to enter the passphrase for the network
SSID = raw_input("\nPlease enter the SSID of the network to which you would like to connect, then press Enter:\n")

# Determine if the network credentials are already stored in wpa_supplicant.conf
wpa_conf = open("/etc/wpa_supplicant.conf", "a+")
ssid_exists = "false"
for line in wpa_conf:
	if SSID in line:
		ssid_exists = "true"

# If credentials do not already exist, prompt the user to enter passphrase for the network
if ssid_exists == "false":
	print "\nPlease enter the passphrase for the network, then press Enter:"
	subprocess.call(["wpa_passphrase", SSID], stdout=wpa_conf)
	print

if os.path.isfile("/var/run/wpa_supplicant/wlan0") == True:
	subprocess.call(["rm", "/var/run/wpa_supplicant/wlan0"])

# Start wpa_supplicant and obtain IP address from AP
print
subprocess.call(["wpa_supplicant", "-B", "-i", "wlan0", "-c", "/etc/wpa_supplicant.conf", "-D", "nl80211"])
subprocess.call(["udhcpc", "-i", "wlan0"])
wpa_conf.close()

#return rtn_val
