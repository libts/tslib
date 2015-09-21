/*
 *  tslib/src/ts_uinput_touch.c
 *
 *  Copyright (C) 2001 Russell King.
 *  Copyright (C) 2015 Peter Vicman.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Takes tslib events and send them to uinput as
 *   BTN_TOUCH, BTN_RIGHT, ABS_X, ABS_Y and ABS_PRESSURE values.
 * Short tap send button touch event, little longer tap send button right event
 *   and longer tap send only coordinates.
 * Tested only with Kodi application.
 *
 * code based on
 *   tslib/src/ts_test.c
 *   tslib/src/ts_calibrate.c
 *   http://thiemonge.org/getting-started-with-uinput
 *   http://lkcl.net/software/uinput/
 *
 * In memory of my mom and dad ...
 */

#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include "tslib.h"
#include "testutils.h"

#define die(str, args...) do { \
    perror(str); \
    exit(EXIT_FAILURE); \
  } while(0);

#define SET_ABS_MINMAX(dev, arg, min, max) do { \
    (dev).absmin[arg] = min; \
    (dev).absmax[arg] = max; \
  } while(0);

#define CANCEL_CALIBRATION if (! calibration_mode) return; else {}

#define SOCK_PATH "/tmp/ts_calibrate.socket"

#define CROSSHAIR_STR1 "Top left"
#define CROSSHAIR_STR2 "Top right"
#define CROSSHAIR_STR3 "Bottom right"
#define CROSSHAIR_STR4 "Bottom left"
#define CROSSHAIR_STR5 "Center"

typedef struct {
  int x[5], xfb[5];
  int y[5], yfb[5];
  int a[7];
} calibration;

struct tsdev *ts = NULL;
char *uinput_names[] = {"/dev/uinput", "/dev/input/uinput", "/dev/misc/uinput"};
#define UINPUT_NAMES_NUM ((int) (sizeof(uinput_names)/sizeof(char *)))

__u32 xres = 0, yres = 0;
struct timeval tv_short_tap = {0, 300};  /* sec, msec, short tap sends BTN_TOUCH */
struct timeval tv_right_tap = {1, 0};    /* sec, msec, longer tap sends BTN_RIGHT */
bool calibration_mode = false;
int sock = -1;

static void daemonize() {
  pid_t pid;

  pid = fork();
  if (pid == -1) {
    die("failed to fork while daemonising (errno=%d)", errno)
  } else if (pid != 0) {
    exit(0);
  }

  umask(0);

  if (setsid() == -1) {
    die("failed to become a session leader while daemonising(errno=%d)", errno)
  }

  /* signal(SIGHUP,SIG_IGN); */
  pid=fork();
  if (pid == -1) {
    die("failed to fork while daemonizing (errno=%d)", errno)
  } else if (pid != 0) {
    exit(0);
  }

  if (chdir("/") == -1) {
    die("failed to change working directory while daemonizing (errno=%d)", errno)
  }

  umask(0);
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  if (open("/dev/null", O_RDONLY) == -1) {
    die("failed to reopen stdin while daemonizing (errno=%d)", errno)
  }

  if (open("/dev/null", O_WRONLY) == -1) {
    die("failed to reopen stdout while daemonizing (errno=%d)", errno)
  }

  if (open("/dev/null", O_RDWR) == -1) {
    die("failed to reopen stderr while daemonizing (errno=%d)", errno)
  }
}

static void signal_end(int sig)
{
  fflush(stderr);
  printf("signal %d caught\n", sig);
  fflush(stdout);
  exit(1);
}

static void set_ioctl(int fd, unsigned int cmd, __u16 value)
{
  if (ioctl(fd, cmd, value) < 0)
    die("error: ioctl")
}

static int send_event(int fd, __u16 type, __u16 code, __s32 value)
{
  struct input_event event;
  int ret;

  memset(&event, 0, sizeof(event));
  event.type = type;
  event.code = code;
  event.value = value;

  ret = write(fd, &event, sizeof(event));
  if (ret != sizeof(event)) {
    fprintf(stderr, "Error on send_event");
    return -1;
  }

  return 0;
}

static void signal_handler(int signal_number) {
	if (signal_number == SIGHUP) {
  	/* reload modules (and calibration data) */
  	printf("signal handler %d, reconfig ts\n", signal_number);
  	if (ts_reconfig(ts)) {
  	  die("ts_reconfig")
	  }
	  
	  return;
	} else if (signal_number == SIGUSR1)
    calibration_mode = true;
  else if (signal_number == SIGUSR2)
    calibration_mode = false;
  else
    return;

  printf("signal handler %d, current calibration_mode=%d\n", signal_number, calibration_mode == true ? 1 : 0);
}

static int perform_calibration(calibration *cal) {
  int j;
  float n, x, y, x2, y2, xy, z, zx, zy;
  float det, a, b, c, e, f, i;
  float scaling = 65536.0;

// Get sums for matrix
  n = x = y = x2 = y2 = xy = 0;
  for(j=0;j<5;j++) {
    n += 1.0;
    x += (float)cal->x[j];
    y += (float)cal->y[j];
    x2 += (float)(cal->x[j]*cal->x[j]);
    y2 += (float)(cal->y[j]*cal->y[j]);
    xy += (float)(cal->x[j]*cal->y[j]);
  }

// Get determinant of matrix -- check if determinant is too small
  det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
  if(det < 0.1 && det > -0.1) {
    printf("ts_calibrate: determinant is too small -- %f\n",det);
    return 0;
  }

// Get elements of inverse matrix
  a = (x2*y2 - xy*xy)/det;
  b = (xy*y - x*y2)/det;
  c = (x*xy - y*x2)/det;
  e = (n*y2 - y*y)/det;
  f = (x*y - n*xy)/det;
  i = (n*x2 - x*x)/det;

// Get sums for x calibration
  z = zx = zy = 0;
  for(j=0;j<5;j++) {
    z += (float)cal->xfb[j];
    zx += (float)(cal->xfb[j]*cal->x[j]);
    zy += (float)(cal->xfb[j]*cal->y[j]);
  }

// Now multiply out to get the calibration for framebuffer x coord
  cal->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
  cal->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
  cal->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));

  printf("%f %f %f\n",(a*z + b*zx + c*zy),
        (b*z + e*zx + f*zy),
        (c*z + f*zx + i*zy));

// Get sums for y calibration
  z = zx = zy = 0;
  for(j=0;j<5;j++) {
    z += (float)cal->yfb[j];
    zx += (float)(cal->yfb[j]*cal->x[j]);
    zy += (float)(cal->yfb[j]*cal->y[j]);
  }

// Now multiply out to get the calibration for framebuffer y coord
  cal->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
  cal->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
  cal->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));

  printf("%f %f %f\n",(a*z + b*zx + c*zy),
        (b*z + e*zx + f*zy),
        (c*z + f*zx + i*zy));

// If we got here, we're OK, so assign scaling to a[6] and return
  cal->a[6] = (int)scaling;
  return 1;
/*
// This code was here originally to just insert default values
  for(j=0;j<7;j++) {
    c->a[j]=0;
  }
  c->a[1] = c->a[5] = c->a[6] = 1;
  return 1;
*/
}

static void get_sample(struct tsdev *ts, calibration *cal,
      int index, int x, int y, char *name)
{
  printf("getting sample for: %s\n", name);
  getxy(ts, &cal->x[index], &cal->y[index]);
  cal->xfb[index] = x;
  cal->yfb[index] = y;
  printf("%s: X = %4d Y = %4d\n", name, cal->x[index], cal->y[index]);
}

static void clearbuf(struct tsdev *ts)
{
  int fd = ts_fd(ts);
  fd_set fdset;
  struct timeval tv;
  int nfds;
  struct ts_sample sample;

  while (1) {
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    nfds = select(fd + 1, &fdset, NULL, NULL, &tv);
    if (nfds == 0) break;

    if (ts_read_raw(ts, &sample, 1) < 0) {
      perror("ts_read");
      exit(1);
    }
  }
}

/* we don't use fb but this function is called in case of an error */
void close_framebuffer(void) {}

static void send_socket_crosshair_str(char *str)
{
  if (send(sock, str, strlen(str), 0) == -1) {
    printf("send error\n");
  }
}

static void run_calibration(struct tsdev *ts)
{
  calibration cal;
  int cal_fd;
  char cal_buffer[256];
  char *calfile;
  unsigned int i, len;

  send_socket_crosshair_str(CROSSHAIR_STR1);  /* show first touch point */
  clearbuf(ts);
  get_sample(ts, &cal, 0, 50,        50,        CROSSHAIR_STR1);
  CANCEL_CALIBRATION
  send_socket_crosshair_str(CROSSHAIR_STR2);
  clearbuf(ts);
  get_sample(ts, &cal, 1, xres - 50, 50,        CROSSHAIR_STR2);
  CANCEL_CALIBRATION
  send_socket_crosshair_str(CROSSHAIR_STR3);
  clearbuf(ts);
  get_sample(ts, &cal, 2, xres - 50, yres - 50, CROSSHAIR_STR3);
  CANCEL_CALIBRATION
  send_socket_crosshair_str(CROSSHAIR_STR4);
  clearbuf(ts);
  get_sample(ts, &cal, 3, 50,        yres - 50, CROSSHAIR_STR4);
  CANCEL_CALIBRATION
  send_socket_crosshair_str(CROSSHAIR_STR5);
  clearbuf(ts);
  get_sample(ts, &cal, 4, xres / 2,  yres / 2,  CROSSHAIR_STR5);
  CANCEL_CALIBRATION
  send_socket_crosshair_str("done");
  clearbuf(ts);

  if (perform_calibration(&cal)) {
    printf ("Calibration constants: ");
    for (i = 0; i < 7; i++)
      printf("%d ", cal.a[i]);

    printf("\n");
    calfile = getenv("TSLIB_CALIBFILE");
    if (calfile != NULL) {
      cal_fd = open(calfile, O_CREAT | O_TRUNC | O_RDWR,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

      if (cal_fd == -1) {
        printf("Calibration failed - can't open calibration file %s.\n", calfile);
        return;
      }
    } else {
      cal_fd = open(TS_POINTERCAL, O_CREAT | O_TRUNC | O_RDWR,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

      if (cal_fd == -1) {
        printf("Calibration failed - can't open calibration file %s.\n", TS_POINTERCAL);
        return;
      }
    }

    len = sprintf(cal_buffer, "%d %d %d %d %d %d %d %d %d",
                  cal.a[1], cal.a[2], cal.a[0],
                  cal.a[4], cal.a[5], cal.a[3], cal.a[6],
                  xres, yres);
    write(cal_fd, cal_buffer, len);
    close(cal_fd);
  } else {
    printf("Calibration failed - wrong data.\n");
  }
}

int get_resolution(void)
{
  char *env_str;
  int fd;
  static struct fb_var_screeninfo var;

  env_str = getenv("TSLIB_FBDEVICE");
  if (env_str == NULL)
    goto not_found;

  printf("using fb device: %s\n", env_str);
  fd = open(env_str, O_RDWR);
  if (fd == -1) {
    perror("open fbdevice");
    goto not_found;
  }

  if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
    perror("ioctl FBIOGET_VSCREENINFO");
    close(fd);
    goto not_found;
  }

  xres = var.xres;
  yres = var.yres;

  close(fd);
  return 0;

not_found:
	env_str = getenv("TSLIB_RES_X");
	if (env_str != NULL)
	  xres = atoi(env_str);

	env_str = getenv("TSLIB_RES_Y");
	if (env_str != NULL)
	  yres = atoi(env_str);
	
	if (xres == 0 || yres == 0) {
		xres = yres = 0;
		return -1;
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
  int c;
  char *tsdevice;
  int ret;
  struct ts_sample samp;
  int uinput_fd;
  struct uinput_user_dev uidev;
  int daemon = 0;
  struct timeval tv_last;
  struct timeval tv_sub;
  char *touch_str;
  int val;
  struct sigaction sa;
  struct sockaddr_un remote_addr;

  touch_str = getenv("TSLIB_TAP_TIME");
  if (touch_str != NULL) {
    val = atoi(touch_str);
    if (val < 1000) {
      tv_short_tap.tv_sec = 0;
      tv_short_tap.tv_usec = val;
    } else {
      tv_short_tap.tv_sec = val / 1000;
      tv_short_tap.tv_usec = val % 1000;
    }
  }

  ret = get_resolution();

  while ((c = getopt(argc, argv, "?dt:x:y:")) != -1) {
    switch (c) {
      case 'd':
        daemon = 1;
        break;
      case 't':
        val = atoi(optarg);
        if (val < 1000) {
          tv_short_tap.tv_sec = 0;
          tv_short_tap.tv_usec = val;
        } else {
          tv_short_tap.tv_sec = val / 1000;
          tv_short_tap.tv_usec = val % 1000;
        }

        break;
      case 'x':
        xres = atoi(optarg);
        break;
      case 'y':
        yres = atoi(optarg);
        break;
      case '?':
        if (isprint(optopt))
          fprintf (stderr, "Unknown option '-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character '\\x%x'.\n", optopt);
        return 1;
      default:
        die("getopt")
    }
  }

  printf("resolution: %dx%d\n", xres, yres);

  signal(SIGSEGV, signal_end);
  signal(SIGINT, signal_end);
  signal(SIGTERM, signal_end);

  tsdevice = getenv("TSLIB_TSDEVICE");
  if (tsdevice == NULL) {
    errno = ENOENT;
    die("error: TSLIB_TSDEVICE")
  }

  printf("using touch device: %s\n", tsdevice);

  ts = ts_open(tsdevice, 0);  // 0 nonblock
  if (!ts) {
    die("ts_open")
  }

  if (ts_config(ts)) {
    die("ts_config")
  }

  ret = system("modprobe uinput");

  for (c=0; c < UINPUT_NAMES_NUM; c++) {
    uinput_fd = open(uinput_names[c], O_WRONLY | O_NONBLOCK);
    if (uinput_fd >= 0)
      break;
  }

  if (uinput_fd < 0)
    die("error: opening uinput")

  if (tv_short_tap.tv_sec == 0)
    printf("Short tap time: %li msec\n", tv_short_tap.tv_usec);
  else
    printf("Short tap time: %li.%li sec\n", tv_short_tap.tv_sec, tv_short_tap.tv_usec);

  if (tv_right_tap.tv_sec == 0)
    printf("Right tap time: %li msec\n", tv_right_tap.tv_usec);
  else
    printf("Right tap time: %li.%li sec\n", tv_right_tap.tv_sec, tv_right_tap.tv_usec);

  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &signal_handler;
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);

  if (daemon == 1) {
    printf("daemonizing...\n");
    daemonize();
  }

  set_ioctl(uinput_fd, UI_SET_EVBIT,  EV_SYN);
  set_ioctl(uinput_fd, UI_SET_EVBIT,  EV_KEY);
  set_ioctl(uinput_fd, UI_SET_KEYBIT, BTN_TOUCH);
  set_ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
  set_ioctl(uinput_fd, UI_SET_EVBIT,  EV_ABS);
  set_ioctl(uinput_fd, UI_SET_ABSBIT, ABS_X);
  set_ioctl(uinput_fd, UI_SET_ABSBIT, ABS_Y);
  set_ioctl(uinput_fd, UI_SET_ABSBIT, ABS_PRESSURE);

  memset(&uidev, 0, sizeof(uidev));

  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "tslib to uinput daemon");
  SET_ABS_MINMAX(uidev, ABS_X, 0, xres)
  SET_ABS_MINMAX(uidev, ABS_Y, 0, yres)
  SET_ABS_MINMAX(uidev, ABS_PRESSURE, 0, 255)

  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 0x34a5;
  uidev.id.product = 0x67b8;
  uidev.id.version = 1;

  if (write(uinput_fd, &uidev, sizeof(uidev)) < 0)
    die("error: write")

  if (ioctl(uinput_fd, UI_DEV_CREATE) < 0)
    die("error: ioctl UI_DEV_CREATE")

  tv_short_tap.tv_usec *= 1000;   /* msec to usec */
  tv_right_tap.tv_usec *= 1000;   /* msec to usec */

  timerclear(&tv_last);
  while (1) {
    touch_str = "";
    if (calibration_mode) {
      usleep(250000);   /* app become ready */
      printf("calibration mode started\n");

      sock = socket(AF_UNIX, SOCK_STREAM, 0);
      if (sock == -1) {
        die("socket");
      }

      printf("trying to connect\n");

      remote_addr.sun_family = AF_UNIX;
      strcpy(remote_addr.sun_path, SOCK_PATH);
      val = strlen(remote_addr.sun_path) + sizeof(remote_addr.sun_family);
      if (connect(sock, (struct sockaddr *) &remote_addr, val) == -1) {
        calibration_mode = false;
        close(sock);
        printf("connect error, skip calibration\n");
        usleep(200000);
        continue;
      }

      printf("connected\n");
      run_calibration(ts);
      calibration_mode = false;
      close(sock);
      printf("calibration mode finished, reload plugins\n");

      clearbuf(ts);
      if (ts_reconfig(ts)) {
        die("ts_reconfig")
      }
    }

    ret = ts_read(ts, &samp, 1);
    if (ret < 0) {
      usleep(200000);
      continue;
    } else if (ret == 0) {
      /* can't grab ts device */
      usleep(400000);
    } else if (ret != 1)
      continue;

    send_event(uinput_fd, EV_ABS, ABS_X, samp.x);
    send_event(uinput_fd, EV_ABS, ABS_Y, samp.y);

    if (samp.pressure > 0) {  /* touched */
      send_event(uinput_fd, EV_ABS, ABS_PRESSURE, 255);
      send_event(uinput_fd, EV_SYN, 0, 0);

      if (timerisset(&tv_last) == false) {
        memcpy(&tv_last, &samp.tv, sizeof(struct timeval)); /* touched first time */
        touch_str = "touched first";
      }
    } else {  /* released */
      timersub(&samp.tv, &tv_last, &tv_sub);
      if (timercmp(&tv_sub, &tv_short_tap, <=)) {
        send_event(uinput_fd, EV_ABS, ABS_PRESSURE, 255);
        send_event(uinput_fd, EV_KEY, BTN_TOUCH, 1);
        send_event(uinput_fd, EV_SYN, 0, 0);
        send_event(uinput_fd, EV_ABS, ABS_PRESSURE, 0);
        send_event(uinput_fd, EV_KEY, BTN_TOUCH, 0);
        send_event(uinput_fd, EV_SYN, 0, 0);

        touch_str = "released and send tap";
      } else if (timercmp(&tv_sub, &tv_right_tap, <=)) {
        send_event(uinput_fd, EV_ABS, ABS_PRESSURE, 255);
        send_event(uinput_fd, EV_KEY, BTN_RIGHT, 1);
        send_event(uinput_fd, EV_SYN, 0, 0);
        send_event(uinput_fd, EV_ABS, ABS_PRESSURE, 0);
        send_event(uinput_fd, EV_KEY, BTN_RIGHT, 0);
        send_event(uinput_fd, EV_SYN, 0, 0);

        touch_str = "released and send right";
      } else {
        send_event(uinput_fd, EV_ABS, ABS_PRESSURE, 0);
        send_event(uinput_fd, EV_SYN, 0, 0);
        touch_str = "released";
      }

      timerclear(&tv_last);
    }

    printf("%ld.%06ld: %6d %6d %6d  %s\n", samp.tv.tv_sec, samp.tv.tv_usec,
          samp.x, samp.y, samp.pressure, touch_str);
  }

  if (ioctl(uinput_fd, UI_DEV_DESTROY) < 0)
    die("error: ioctl UI_DEV_DESTROY")

  close(uinput_fd);

  return 0;
}
