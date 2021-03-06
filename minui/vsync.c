/*
 * Copyright (c) 2014, The CyanogenMod Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <pthread.h>
#include <time.h>

#include <linux/ioctl.h>
#include <linux/msm_mdp.h>

#define USEC_PER_SEC    (1000*1000)
#define NSEC_PER_SEC    (1000*USEC_PER_SEC)

#define FB_NUM 0
#define VSYNC_PREFIX "VSYNC="
#define VSYNC_TIMEOUT_NS (60 * 1000 * 1000)

static pthread_cond_t vsync;
static pthread_mutex_t vsync_lock = PTHREAD_MUTEX_INITIALIZER;
static int vsync_enabled = 0;
static struct timespec vsync_time;

static int fb_fd = -1;

static struct timespec ts_add_nsec(struct timespec* lhs, long nsec)
{
    struct timespec result = *lhs;
    result.tv_sec = lhs->tv_sec + (nsec / NSEC_PER_SEC);
    result.tv_nsec = lhs->tv_nsec + (nsec % NSEC_PER_SEC);
    if (result.tv_nsec >= NSEC_PER_SEC) {
        result.tv_sec++;
        result.tv_nsec -= NSEC_PER_SEC;
    }
    return result;
}

static struct timespec ts_sub(struct timespec* lhs, struct timespec* rhs)
{
    struct timespec result;
    result.tv_sec = lhs->tv_sec - rhs->tv_sec;
    result.tv_nsec = lhs->tv_nsec - rhs->tv_nsec;
    if (result.tv_nsec < 0) {
        result.tv_sec--;
        result.tv_nsec += NSEC_PER_SEC;
    }
    return result;
}

static int vsync_control(int enable)
{
    int ret = 0;

    // save the time so we can turn off the interrupt when idle
    clock_gettime(CLOCK_MONOTONIC, &vsync_time);

    if (vsync_enabled != enable) {
        if (fb_fd <= 0 || ioctl(fb_fd, MSMFB_OVERLAY_VSYNC_CTRL, &enable) < 0) {
            perror("vsync control failed!");
            ret = -errno;
        } else {
            vsync_enabled = enable;
        }
    }

    return ret;
}

static void *vsync_loop(void *data)
{
    char vsync_node_path[255];
    char vdata[64];
    int fd = -1;
    int err = 0, len = 0;
    struct timespec now, diff;
    struct pollfd pfd;

    snprintf(vsync_node_path, sizeof(vsync_node_path),
            "/sys/class/graphics/fb%d/vsync_event", 0);
    fd = open(vsync_node_path, O_RDONLY);

    if (fd < 0) {
        perror("unable to initialize vsync\n");
        return NULL;
    }

    pread(fd, vdata, 64, 0);
    pfd.fd = fd;
    pfd.events = POLLPRI | POLLERR;

    printf("%s: vsync thread started\n", __func__);

    // loop forever until sysfs wakes us up
    while (true) {
        err = poll(&pfd, 1, -1);
        if (err <= 0) {
            continue;
        }

        if (pfd.revents & POLLPRI) {
            len = pread(pfd.fd, vdata, 64, 0);
            if (len > 0) {
                // notify waiters
                if (!strncmp(vdata, VSYNC_PREFIX, strlen(VSYNC_PREFIX))) {
                    pthread_cond_signal(&vsync);
                }
            } else {
                perror("unable to read vsync timestamp!\n");
            }
        }

        // turn of the interrupt when we're not drawing
        clock_gettime(CLOCK_MONOTONIC, &now);
        diff = ts_sub(&now, &vsync_time);

        if (diff.tv_sec > 0 || (diff.tv_nsec > VSYNC_TIMEOUT_NS)) {
            vsync_control(0);
        }
    }
}

void wait_for_vsync()
{
    static struct timespec now, timeout;
    int ret = 0;

    vsync_control(1);

    clock_gettime(CLOCK_MONOTONIC, &now);
    timeout = ts_add_nsec(&now, 20 * 1000 * 1000);

    // vsync_loop will let us know when to proceed
    pthread_mutex_lock(&vsync_lock);
    pthread_cond_timedwait(&vsync, &vsync_lock, &timeout);
    pthread_mutex_unlock(&vsync_lock);
}

int vsync_init(int fd)
{
    pthread_t vsync_thread;
    int ret = 0;

    fb_fd = fd;

    pthread_mutex_init(&vsync_lock, NULL);
    pthread_cond_init(&vsync, NULL);

    ret = pthread_create(&vsync_thread, NULL, vsync_loop, NULL);

    if (ret != 0) {
        perror("failed to create vsync thread!");
    }

    return ret;
}
