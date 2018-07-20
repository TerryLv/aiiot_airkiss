#ifndef __WIFI_SCAN__
#define __WIFI_SCAN__

#include <iwlib.h>

#define MAX_ESSID_SIZE 255
#define MAX_BSSID_SIZE 18

#define MAX_CHANNELS 14

int32_t  ak_wifi_scan_do_scan(char *wi_if_name, int32_t *det_channels, int32_t *chan_num);

#endif
