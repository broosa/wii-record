#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <poll.h>

#include <sys/time.h>

#include <xwiimote.h>

typedef struct  {
    int32_t x;
    int32_t y;
    int32_t z;
} vec3;

int timeout = 5000;
int poll_rate = 1000;
FILE *out_file;
char *f_name;

int parse_cmdline(int argc, char *argv[])
{
    out_file = stdout;
    char opt_char = getopt(argc, argv, "t:p:f:");

    //Parse command line options
    while (opt_char != -1) {
        switch (opt_char) {
        case 't':
            timeout = atoi(optarg);
            break;
        case 'p':
            poll_rate = atoi(optarg);
            break;
        case 'f':
            f_name = optarg;
            out_file = fopen(f_name, "w");
            if (out_file == NULL) {
                perror("IO Error");
                return -1;
            }

            printf("Output filename: %s\n", f_name);
        }
    }

    return 0;
}

long get_time_ms(struct timeval *t)
{
    return t->tv_sec * 1000 + t->tv_usec / 1000;
}

vec3 extract_vector(struct xwii_event *evt, vec3 *v)
{
    struct xwii_event_abs payload = evt->v.abs[0];
    v->x = payload.x;
    v->y = payload.y;
    v->z = payload.z;
}


int main(int argc, char *argv[])
{
    int cmd_result = parse_cmdline(argc, argv);
    if (cmd_result != 0) {
        return -1;
    }

    //Start searching for wiimotes
    struct xwii_monitor *wii_mon = xwii_monitor_new(true, false);
    int wii_mon_fd = xwii_monitor_get_fd(wii_mon, true);

    struct pollfd poll_fds[2];
    memset(poll_fds, 0, sizeof(poll_fds));

    poll_fds[0].fd = wii_mon_fd;
    poll_fds[0].events = POLLIN;

    char *wii_dev_name = xwii_monitor_poll(wii_mon);

    if (wii_dev_name == NULL) {
        printf("No Wiimotes found. Exiting.\n");
        return -1;
    }

    printf("Found Wiimote: %s\n", wii_dev_name);

    ///Using the device name, set up a new device to poll from
    struct xwii_iface *wii_dev;
    int result = xwii_iface_new(&wii_dev, wii_dev_name);

    if (result != 0) {
        fprintf(stderr, "Error creating Wiimote interface: %d\n", result);
        return -1;
    }

    printf("Enabling sensor interfaces...\n");

    unsigned int iface_mask = XWII_IFACE_CORE | XWII_IFACE_ACCEL | XWII_IFACE_IR |
                        XWII_IFACE_MOTION_PLUS;

    xwii_iface_open(wii_dev, iface_mask);
    xwii_iface_watch(wii_dev, true);
    int wii_dev_fd = xwii_iface_get_fd(wii_dev);

    poll_fds[1].fd = wii_dev_fd;
    poll_fds[1].events = POLLIN;

    while (true) {
        poll(poll_fds, 2, 1000 / poll_rate);
        struct xwii_event wii_evt;
        //Retrieve an event from the kernel if available
        if (poll_fds[1].revents & POLLIN) {
            int events_ready = xwii_iface_dispatch(wii_dev, &wii_evt, sizeof(wii_evt));
            while (events_ready == 0) {
                if (wii_evt.type == XWII_EVENT_ACCEL || wii_evt.type == XWII_EVENT_MOTION_PLUS) {
                    //Get a timestamp for this measurement
                    long time_ms = get_time_ms(&wii_evt.time);
                    vec3 v;
                    extract_vector(&wii_evt, &v);

                    printf("%d,%ld,%d,%d,%d\n", wii_evt.type, time_ms, v.x, v.y, v.z);
                }
                events_ready = xwii_iface_dispatch(wii_dev, &wii_evt, sizeof(wii_evt));
            }
        }
    }

    //Clean up files and pointers
    xwii_iface_unref(wii_dev);
    xwii_monitor_unref(wii_mon);

    if (out_file != stdout) {
        fclose(out_file);
    }
 }
