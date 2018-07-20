#include "common.h"
#include "wifi_scan.h"
#include "libcrc/checksum.h"
#include <iwlib.h>

static iwrange range;
static int32_t gTmpChanNum = 0;

static int32_t ak_wifi_scan_for_ap(char *device, wireless_scan_head *result_head)
{
    int32_t sock;
    int32_t success = -1;

    /* Open socket to kernel */
    sock = iw_sockets_open();

    do {
        if(sock < 0) {
            LOG_ERROR("Error to open kernel sockect.");
            break;
        }

        /* Get some metadata to use for scanning */
        if (iw_get_range_info(sock, device, &range) < 0) {
            LOG_ERROR("Error during iw_get_range_info.");
            break;
        }

        /* Perform the scan */
        if (iw_scan(sock, device, range.we_version_compiled, result_head) < 0) {
            LOG_ERROR("Error during iw_scan.");
            break;
        }

        success = 0;
    } while(0);

    iw_sockets_close(sock);
    return success;
}

static void ak_wifi_scan_get_essid(wireless_scan *ap, char *essid, uint32_t len)
{
    snprintf(essid, len, "%s", ap->b.essid);
}

static void ak_wifi_scan_get_bssid(wireless_scan *ap, char *bssid, uint32_t len)
{
    uint8_t mac_addr[7];
    memcpy(mac_addr, ap->ap_addr.sa_data, 7);
    snprintf(bssid, len,
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0],
            mac_addr[1],
            mac_addr[2],
            mac_addr[3],
            mac_addr[4],
            mac_addr[5]
            );
}

uint32_t ak_wifi_scan_get_channel(wireless_scan *ap)
{
	if (ap)
		return ap->b.freq;
	else
		return 0;
}

uint32_t ak_wifi_scan_get_freq_mhz(wireless_scan *ap)
{
    uint32_t freq = 0;
    if(ap)
        freq = ap->b.freq;
    return freq;
}

int32_t ak_wifi_scan_get_strength_dbm(wireless_scan *ap)
{
    char buf[128] = { 0 };
    int32_t qual,qual_all,strength;

    iw_print_stats(buf, 128, &ap->stats.qual, &range, 1);
    sscanf(buf, "Quality=%d/%d  Signal level=%d dBm  ",
            &qual,&qual_all,&strength);
    return strength;
}


void ak_wifi_scan_print_ap_info(wireless_scan *ap)
{
    char essid[MAX_ESSID_SIZE] = { 0 };
    char bssid[MAX_BSSID_SIZE] = { 0 };
    uint32_t freq = 0;
    int32_t strength = 0;

    ak_wifi_scan_get_essid(ap, essid, MAX_ESSID_SIZE);
    ak_wifi_scan_get_bssid(ap, bssid, MAX_BSSID_SIZE);
    freq = ak_wifi_scan_get_freq_mhz(ap);
    strength = ak_wifi_scan_get_strength_dbm(ap);

    LOG_TRACE("bssid:[%s], freq:[%d MHz], power:[%d dBm], essid:[%s]\n",
            bssid,
            freq,
            strength,
            essid);
}

static void ak_wifi_scan_add_channel(int32_t *channels, int32_t chan)
{
    int32_t i;

    for (i = 0; i < gTmpChanNum; i++) {
        if (channels[i] == chan)
            break;
    }
    if (i == gTmpChanNum) {
        channels[gTmpChanNum++] = chan;
    }
}

static void ak_wifi_scan_reset_channels(int32_t *det_channels)
{
    int32_t i;

    for (i = 1; i < MAX_CHANNELS; i++) {
        ak_wifi_scan_add_channel(det_channels, i);
	}
	gTmpChanNum = MAX_CHANNELS;
}

int32_t ak_wifi_scan_do_scan(char *wi_if_name, int32_t *det_channels, int32_t *chan_num)
{
	wireless_scan_head head;
	wireless_scan *presult = NULL;

	LOG_TRACE("Scanning accesss point...");
    if(ak_wifi_scan_for_ap(wi_if_name, &head) == 0)
    {
        LOG_TRACE("Scan success.");
        presult = head.result;
        while(presult != NULL) {
            char essid[MAX_ESSID_SIZE] = { 0 };
            char bssid[MAX_BSSID_SIZE] = { 0 };
			//unsigned int freq_index = 0;
			int32_t channel = 0, power = 0;
			uint8_t essid_crc = 0;

            ak_wifi_scan_get_essid(presult, essid, MAX_ESSID_SIZE);
            ak_wifi_scan_get_bssid(presult, bssid, MAX_BSSID_SIZE);
			channel = ak_wifi_scan_get_channel(presult);
			power   = ak_wifi_scan_get_strength_dbm(presult);
			essid_crc = crc_8((const unsigned char *)essid, strlen((char *)essid));

            LOG_TRACE("bssid:[%s], channel:[%2d], pow:[%d dBm], essid_crc:[%02x], essid:[%s]",
                    bssid, channel, power, essid_crc, essid);
            ak_wifi_scan_add_channel(det_channels, channel);
            presult = presult->next;
        }
    }
    else
    {
        LOG_ERROR("ERROR to scan AP, init with all %d channels", MAX_CHANNELS);
        ak_wifi_scan_reset_channels(det_channels);
    }
	*chan_num = gTmpChanNum;
	return 0;
}

