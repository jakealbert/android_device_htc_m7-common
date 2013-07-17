/*
 * Copyright (C) 2013 Cyanogenmod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_TAG "irda"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/irda.h>

#define CIR_DEVICE "/dev/ttyHSL2"
#define CIR_RESET "/sys/class/htc_cir/cir/reset_cir"

static int irda_enable(int on)
{
    int fd, ret = 0;
    size_t bytes_written;
    char enable;

    /* Don't ask me why 0 == off, 1 == reset, 2 == on */
    enable = on ? '2' : '0';

    fd = open(CIR_RESET, O_WRONLY);
    if (fd < 0) {
        ALOGE("%s: Error opening %s: %d\n", __func__, CIR_RESET, errno);
        ret = -EINVAL;
        goto enable_err;
    }

    bytes_written = write(fd, &enable, 1);
    if (bytes_written != 1) {
        ALOGE("%s: Wrote %u bytes of %u!\n", __func__, bytes_written, 1);
        ret = -ECOMM;
        goto enable_err1;
    }

enable_err1:
    close(fd);
enable_err:
    return ret;
}

static int irda_reset(void)
{
    int fd, ret;
    size_t bytes_written;
    char reset = '1';

    /* Reset Irda device */
    fd = open(CIR_RESET, O_WRONLY);
    if (fd < 0) {
        ALOGE("%s: Error opening %s: %d\n", __func__, CIR_RESET, errno);
        ret = -EINVAL;
        goto reset_err;
    }

    bytes_written = write(fd, &reset, 1);
    if (bytes_written != 1) {
        ALOGE("%s: Wrote %u bytes of %u!\n", __func__, bytes_written, 1);
        ret = -ECOMM;
        goto reset_err1;
    }

reset_err1:
    close(fd);
reset_err:
    return ret;
}

static int irda_send(char *buf, size_t len)
{
    int fd, ret = 0;
    size_t bytes_written;
    struct termios t_opts;
    char wakeup = -69;

    /* TODO: are these necessary? */
    ALOGI("%s: enabling\n", __func__);
    irda_enable(1);
    irda_reset();

    fd = open(CIR_DEVICE, O_WRONLY | O_NOCTTY);
    if (fd < 0) {
        ALOGE("%s: Error opening %s: %d\n", __func__, CIR_DEVICE, errno);
        ret = -EINVAL;
        goto send_err;
    }

    ALOGI("%s: flushing\n", __func__);
    tcflush(fd, TCIOFLUSH);

    if (tcgetattr(fd, &t_opts) < 0) {
        ALOGE("%s: Error getting device attributes: %d\n", __func__, errno);
        ret = -EINVAL;
        goto send_err1;
    }

    t_opts.c_iflag = IGNPAR;
    t_opts.c_oflag = 0;
    t_opts.c_cflag = t_opts.c_cflag & (0xFFFFE600 | 0x18F2);
    t_opts.c_lflag = 0;
    t_opts.c_line = 0;
    //t_opts.c_cc = 0;

    ALOGI("%s: setting attributes\n", __func__);
    if (tcsetattr(fd, TCSANOW, &t_opts) < 0) {
        ALOGE("%s: Error setting device attributes: %d\n", __func__, errno);
        ret = -EINVAL;
        goto send_err1;
    }

    ALOGI("%s: sending wakeup\n", __func__);
    bytes_written = write(fd, &wakeup, 1);
    if (bytes_written != 1) {
        ALOGE("%s: Wrote %u bytes of %u\n", __func__, bytes_written, 1);
        ret = -ECOMM;
        goto send_err1;
    }

    ALOGI("%s: writing buffer\n", __func__);
    bytes_written = write(fd, buf, len);
    if (bytes_written != len) {
        ALOGE("%s: Wrote %u bytes of %u!\n", __func__, bytes_written, len);
        ret = -ECOMM;
        goto send_err1;
    }

send_err1:
    close(fd);
send_err:
    irda_enable(0);
    return ret;
}

static void irda_send_ircode(char *buffer, int length)
{
    int i;
    char buf[length + 1];

    memcpy(buf, buffer, length);
    buf[length] = 0; /* null terminate */
    ALOGI("Received IR buffer %s\n", buf);

    /* HTC expects spaces it seems */
    for (i = 0; i < length; i++) {
        if (buffer[i] == ',') {
            buffer[i] = ' ';
        }
    }

    irda_send(buffer, length);
}

static int open_irda(const struct hw_module_t *module, const char *name,
    struct hw_device_t **device)
{
    struct irda_device_t *dev = malloc(sizeof(struct irda_device_t));
    memset(dev, 0, sizeof(dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *) module;
    dev->send_ircode = irda_send_ircode;

    *device = (struct hw_device_t *) dev;

    return 0;
}

static struct hw_module_methods_t irda_module_methods = {
    .open = open_irda,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = 1,
    .hal_api_version = 0,
    .id = IRDA_HARDWARE_MODULE_ID,
    .name = "Irda HAL Module",
    .author = "The CyanogenMod Project",
    .methods = &irda_module_methods,
};
