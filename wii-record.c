#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <poll.h>

#include <sys/time.h>

#include <xwiimote.h>

int main(int argc, char *argv[])
{
    int timeout = 5000;
    int poll_rate = 100;
    FILE *out_file = stdout;
    char opt_char = getopt(argc, argv, "t:p:f:");

    fd_set sel_fds;

    char *fname;

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
            fname = optarg;
            out_file = fopen(fname, "w");
            if (out_file == NULL) {
                perror("IO Error");
                return -1;
            }

            printf("Output filename: %s\n", fname);
        }
    }

    //Set up a timeout to make selects non-blocking
    //We have the select time out every 100ms to keep cpu usage down
    struct timeval wii_poll_timeout;
    wii_poll_timeout.tv_sec = 5;
    wii_poll_timeout.tv_usec = 1000000 / poll_rate;

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
                printf("Got event with ID: %d\n", wii_evt.type);
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
