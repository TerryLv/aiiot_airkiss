#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <signal.h>

#define RUNTIME_DEBUG

#define KEY_EVENT_DEV_NAME		"/dev/input/event0"
#define AK_KEY_DAMEN_LOG		"./ak_key_daemon.log"

#ifdef RUNTIME_DEBUG
static int32_t run_dbg_flag = 1;
#else
static int32_t run_dbg_flag = 0;
#endif
static FILE *log_fp = NULL;

static struct itimerval key_press_timer;

#define LOG_TRACE(format, args...)	\
{	\
	if (run_dbg_flag)	\
		fprintf(stdout, format, ##args);	\
	else	\
		fprintf(log_fp, format, ##args);	\
}

/* Pressing Power key 3 times in 5 sec is a signal for setting up wifi */
static int32_t key_press_times = 0;
		

/*
void creat_daemon(int nochdir, int noclose)
{
	pid_t pid;

	pid = fork();
	if( pid == -1)
		ERR_EXIT("fork error");
	if(pid > 0 )
		exit(EXIT_SUCCESS);
	if(setsid() == -1)
		ERR_EXIT("SETSID ERROR");
	if(nochdir == 0)
		chdir("/");
	if(noclose == 0) {
		int i;
		for( i = 0; i < 3; ++i) {
			close(i);
			open("/dev/null", O_RDWR);
			dup(0);
			dup(0);
		}
		umask(0);
	return;
}
*/

static int32_t ak_linux_exec_cmd(char *cmd)
{
	FILE *ch_fp = NULL;
	int32_t ret = 0;

	LOG_TRACE("Exec: %s\n", cmd);
	ch_fp = popen(cmd, "w");
	if (!ch_fp) {
		LOG_TRACE("popen channel failed!\n");
		ret = -1;
		goto out;
	}
	ret = pclose(ch_fp);
	if (WIFEXITED(ret));
		LOG_TRACE("subprocess exited, exit code: %d\n", WEXITSTATUS(ret));
out:
	return ret;
}

static int32_t ak_do_reset_wifi(void)
{
	char strbuf[512] = { 0 };

	/* Use setup_wifi_wpa.sh to init wifi connection */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "./reset_wifi_wpa.sh wlan0");
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_TRACE("Cmd execute failed: %s\n", strbuf);
		return -1;
	} else
		return 0;
}

static int32_t ak_do_wifi_airkiss(void)
{
	char strbuf[512] = { 0 };

	/* Use setup_wifi_wpa.sh to init wifi connection */
	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf) - 1, "./aiiot_airkiss -dwlan0");
	if (ak_linux_exec_cmd(strbuf)) {
		LOG_TRACE("Cmd execute failed: %s\n", strbuf);
		return -1;
	} else
		return 0;
}

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

static void ak_key_press_timer_callback(void)
{
	/* Reset flag */
	ak_start_timer(&key_press_timer, 0);
	key_press_times = 0;

}

int32_t main(void)
{
	int32_t key_fd = 0;
	int32_t l_ret = -1;
	struct input_event key_event;

	memset(&key_event, 0, sizeof(struct input_event));

	//log_fp = open(AK_KEY_DAMEN_LOG, O_WRONLY | O_CREAT | O_APPEND, 0644);
	//if (log_fp < 0) {
	log_fp = fopen(AK_KEY_DAMEN_LOG, "a+");
	if (NULL == log_fp) {
		LOG_TRACE("Error open damen log file: %s\n", AK_KEY_DAMEN_LOG);
		return -1;
	}

	key_fd = open(KEY_EVENT_DEV_NAME, O_RDONLY);
	if(key_fd < 0) {
		LOG_TRACE("Error open key device: %s\n", KEY_EVENT_DEV_NAME);
		return -1;
	}

#ifndef RUNTIME_DEBUG
	if(daemon(1,0) == -1)
		ERR_EXIT("daemon error");
#endif

	while(1) {
		l_ret = lseek(key_fd, 0, SEEK_SET);
		l_ret = read(key_fd, &key_event, sizeof(key_event));

		if(l_ret) {
			if(key_event.code == KEY_POWER && key_event.type == EV_KEY
				&& (key_event.value == 0)) {
				LOG_TRACE("key %d %s\n", key_event.code, (key_event.value) ? "pressed" : "released");
				switch (key_press_times++) {
				case 0:
					ak_start_timer(&key_press_timer, 5000);
					signal(SIGALRM, (__sighandler_t)&ak_key_press_timer_callback);
					break;
				case 2:
					LOG_TRACE("Receive WIFI config signal\n");
					ak_do_reset_wifi();
					ak_do_wifi_airkiss();
					break;
				default:
					break;
				}
			}
		}
	}

	close(key_fd);
	fclose(log_fp);

	return 0;
}

