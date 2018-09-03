#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "aircrack-osdep/common.h"
#include "aircrack-osdep/osdep.h"
#include "common.h"
#include "wifi_scan.h"
#include "libcrc/checksum.h"

#include "libak/airkiss.h"

#define MAX_CHANNELS 14
#define AK_PKT_DEBUG 0
#define AK_DEF_WIFI_LEA2

#define WLAN_MODULE_NAME		"qca9377"
#define WLAN_MODULE_PARAM		" con_mode=4"
#define WLAN_DEFAULT_DEVNAME	"wlan0"

static airkiss_context_t *ak_contex = NULL;
static const airkiss_config_t ak_conf = { 
	(airkiss_memset_fn)&memset, 
	(airkiss_memcpy_fn)&memcpy, 
	(airkiss_memcmp_fn)&memcmp, 
	(airkiss_printf_fn)&printf
};

airkiss_result_t ak_result;

static struct itimerval ak_switch_ch_timer;
static struct wif *wi = NULL;

static int32_t gChanIndex = 0;
static int32_t gDetChans[MAX_CHANNELS] = {0};
static int32_t gDetChanNum = 0;
static int32_t gChanIsLocked = 0;
pthread_mutex_t lock;

static int32_t gVerboseEn = 1;
#if AK_PKT_DEBUG
static void ak_dump_pkt_hdr(const uint8_t *packet)
{
	int32_t i;
	uint8_t ch;

	/* print  header */
	LOG_TRACE("len:%4d, airkiss ret:%d [ ", size, ret);
	for (i = 0; i < 24; i++) {
		ch = (uint8_t)(*(packet + i));
		printf("%02x ", ch);
	}
	printf("]\n");
}
#endif

static int32_t ak_start_timer(struct itimerval *timer, int ms)
{
    time_t secs, usecs;
    secs = ms / 1000;
    usecs = ms % 1000 * 1000;

    timer->it_interval.tv_sec = secs;
    timer->it_interval.tv_usec = usecs;
    timer->it_value.tv_sec = secs;
    timer->it_value.tv_usec = usecs;

    setitimer(ITIMER_REAL, timer, NULL);

    return 0;
}

static int32_t ak_linux_exec_cmd(char *cmd)
{
	FILE *ch_fp = NULL;
	int32_t ret = 0;

	LOG_TRACE("Exec: %s", cmd);
	ch_fp = popen(cmd, "w");
	if (!ch_fp) {
		LOG_ERROR("popen channel failed!");
		ret = -1;
		goto out;
	}
	ret = pclose(ch_fp);
	if (WIFEXITED(ret) && gVerboseEn)
		LOG_TRACE("subprocess exited, exit code: %d", WEXITSTATUS(ret));
out:
	return ret;
}

static int32_t ak_set_channel(int channel)
{
	char cmd[128] = { 0 };

	snprintf(cmd, sizeof(cmd) - 1, "iwpriv %s setMonChan %d 0", wi_get_ifname(wi), channel);
	printf("exec cmd: %s\n", cmd);

	if(ak_linux_exec_cmd(cmd))
		LOG_ERROR("cannot set channel to %d\n", channel);
	return 0;
}

static void ak_switch_channel_timer_callback(void)
{
	int32_t ret = 0;

	pthread_mutex_lock(&lock);
	if (gChanIsLocked) {
    	pthread_mutex_unlock(&lock);
		return;
	}
	if (gChanIndex > gDetChanNum - 1) {
		gChanIndex = 0;
		if (gVerboseEn)
        	LOG_TRACE("scaned all channels");
    }

	ret = ak_set_channel(gDetChans[gChanIndex]);
	if (ret)
		LOG_TRACE("cannot set channel to %d", gDetChans[gChanIndex]);

    airkiss_change_channel(ak_contex);
	gChanIndex++;
    pthread_mutex_unlock(&lock);
}

static int32_t ak_do_wifi_conn(char *wifi_dev_name, char *ssid, char *passwd)
{
	char strbuf[512] = { 0 };

	/* Use setup_wifi_wpa.sh to init wifi connection */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "./setup_wifi_wpa.sh %s %s %s",
			wifi_dev_name, ssid, passwd);
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_ERROR("Cmd execute failed: %s\n", strbuf);
		return -1;
	} else
		return 0;
}

static int32_t ak_uninstall_monitor(char *wifi_dev_name)
{
	char strbuf[512] = { 0 };
	int32_t ret = 0;

	/* Set IF down */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "ifconfig %s down", wifi_dev_name);
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_ERROR("Cmd execute failed: %s\n", strbuf);
		//ret = -1;
	}

	/* Del module */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "rmmod %s", WLAN_MODULE_NAME);
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_ERROR("Unable to delete wlan ko: %s\n", strbuf);
		//ret = -1;
	}

	/* Init module */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "modprobe %s", WLAN_MODULE_NAME);
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_ERROR("Unable to install wlan ko: %s\n", strbuf);
		ret = -1;
	}

	/* Set IF up */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "ifconfig %s up", wifi_dev_name);
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_ERROR("Cmd execute failed: %s\n", strbuf);
		ret = -1;
	}

	return ret;
}

static int32_t ak_install_monitor(char *wifi_dev_name)
{
	char strbuf[512] = { 0 };
	int32_t ret = 0;

	/* Set IF down */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "ifconfig %s down", wifi_dev_name);
	if (ak_linux_exec_cmd(strbuf))
		LOG_ERROR("Cmd execute failed: %s, can be ignored\n", strbuf);

	/* Del module */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "rmmod %s", WLAN_MODULE_NAME);
	if (ak_linux_exec_cmd(strbuf))
		LOG_ERROR("Unable to delete wlan ko: %s, can be ignored\n", strbuf);

	/* Init module */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "modprobe %s "WLAN_MODULE_PARAM, WLAN_MODULE_NAME);
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_ERROR("Unable to install wlan ko: %s\n", strbuf);
		ret = -1;
	}

	/* Set IF up */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "ifconfig %s up", wifi_dev_name);
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_ERROR("Cmd execute failed: %s\n", strbuf);
		ret = -1;
	}

	return ret;
}

/* Need to do UDP broadcast to inform the success to Weixin */
int32_t ak_udp_broadcast(uint8_t random, int32_t port)
{
    int32_t fd = 0;
    int32_t enabled = 1;
    int32_t err = 0, i = 0;
    struct sockaddr_in addr;
    useconds_t usecs = 1000 * 20;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons(port);
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        LOG_TRACE("Error to create socket, reason: %s", strerror(errno));
        return 1;
    } 
    
    err = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&enabled, sizeof(enabled));
    if (err == -1) {
        close(fd);
        return -1;
    }
    
	if (gVerboseEn)
    	LOG_TRACE("Sending random to broadcast..");
	for (i = 0; i < 50; i++) {
		sendto(fd, (uint8_t *)&random, 1, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
		usleep(usecs);
	}

    close(fd);
    return 0;
}

static int32_t ak_process_pkt(char *wifi_dev_name, const unsigned char *packet, int size)
{
	int ret;

	pthread_mutex_lock(&lock);
	ret = airkiss_recv(ak_contex, (void *)packet, size);
	switch (ret) {
	case AIRKISS_STATUS_CONTINUE:
		break;
	case AIRKISS_STATUS_CHANNEL_LOCKED:
        ak_start_timer(&ak_switch_ch_timer, 0);
		if (gVerboseEn)
        	LOG_TRACE("lock channel in %d", gDetChans[gChanIndex]);
		gChanIsLocked = 1;
		break;
	case AIRKISS_STATUS_COMPLETE:
		gChanIsLocked = 1;
		if (gVerboseEn)
			LOG_TRACE("Airkiss completed.");
		airkiss_get_result(ak_contex, &ak_result);
		LOG_TRACE("Result:\nssid:[%s]\nssid_len:[%d]\nkey:[%s]\nkey_len:[%d]\nrandom:[0x%02x]", 
			ak_result.ssid,
			ak_result.ssid_length,
			ak_result.pwd,
			ak_result.pwd_length,
			ak_result.random);

		//Save to wpa_suppliant and reset or scan and connect to wifi
		ak_uninstall_monitor(wifi_dev_name);

		// WIFI connection
		if (ak_do_wifi_conn(wifi_dev_name, ak_result.ssid, ak_result.pwd))
			LOG_ERROR("Setup WIFI connection failed!");
		else
			ak_udp_broadcast(ak_result.random, 10000);
		break;
	default:
		//LOG_TRACE("Invalid return from airkiss library: %d!", ret);
		break;
    }

#if AK_PKT_DEBUG
	ak_dump_pkt_hdr(packet);
#endif
    pthread_mutex_unlock(&lock);

	return ret;
}

static void ak_usage(void)
{
	LOG_TRACE("aiiot_airkiss version %s", VERSION);
	LOG_TRACE("Usage is: aiiot_airkiss [options]");
	LOG_TRACE("Options:");
	LOG_TRACE("-v\t\tVerbose (add more v's to be more verbose)");
	LOG_TRACE("-d<device>\tWireless Lan Device (default: "WLAN_DEFAULT_DEVNAME")");
	LOG_TRACE("-c<integer>\tFixed scan channel. In a test environment, this will be more fast");
	LOG_TRACE("-h\tPrint this guide");
    exit (8);
}

int32_t main(int32_t argc, char *argv[])
{
    int32_t result = 0;
    int32_t read_size = 0;
	uint8_t buf[RECV_BUFSIZE] = {0};
	char *wifi_if = WLAN_DEFAULT_DEVNAME;
	int32_t fixed_ch = -1;

	while ((argc > 1) && (argv[1][0] == '-')) {
		switch (argv[1][1]) {
		case 'q':
			gVerboseEn = 0;
			break;
		case 'd':
			wifi_if = &argv[1][2];
			break;
        case 'c':
            fixed_ch = atoi(&argv[1][2]);
            break;
		case 'h':
			ak_usage();
        default:
            LOG_TRACE("Unknown option %s", argv[1]);
            ak_usage();
        }
        ++argv;
        --argc;
    }

	if (fixed_ch <= 0) {
    	LOG_TRACE("Scanning accesss point...");
		ak_wifi_scan_do_scan(wifi_if, gDetChans, &gDetChanNum);
	}

	/* Install ko with monitor support */
	if (ak_install_monitor(wifi_if)) {
		LOG_ERROR("cannot init monitor mode: %s", wifi_if);
        return 1;
	}

    /* Open the interface and set mode monitor */
	wi = wi_open(wifi_if);
	if (!wi) {
		LOG_ERROR("cannot init interface %s", wifi_if);
		return 1;
	}


    /* airkiss setup */
    ak_contex = (airkiss_context_t *)malloc(sizeof(airkiss_context_t));
    result = airkiss_init(ak_contex, &ak_conf);
    if(result != 0)
    {
        LOG_ERROR("Airkiss init failed!!");
        return 1;
    }
    LOG_TRACE("Airkiss version: %s", airkiss_version());
    if(pthread_mutex_init(&lock, NULL) != 0)
    {
        LOG_ERROR("mutex init failed");
        return 1;
	}

	/* Setup channel switch timer */
	gChanIsLocked = 0;
	if (fixed_ch > 0) {
		result = ak_set_channel(fixed_ch);
		if (result)
			LOG_TRACE("cannot set channel to %d", fixed_ch);
		airkiss_change_channel(ak_contex);
	} else {
		ak_start_timer(&ak_switch_ch_timer, 400);   
		signal(SIGALRM, (__sighandler_t)&ak_switch_channel_timer_callback);
	}

	for(;;)
    {
		read_size = wi->wi_read(wi, buf, RECV_BUFSIZE, NULL);
		if (read_size < 0) {
			LOG_ERROR("recv failed, ret %d", read_size);
			break;
		}
        if (AIRKISS_STATUS_COMPLETE == ak_process_pkt(wifi_if, buf, read_size))
            break;
	}

    free(ak_contex);
    pthread_mutex_destroy(&lock);

    return 0;
}
