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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define LOG_TAG "irda"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/irda.h>

static void irda_send_ircode(char *buffer, int length)
{
    char buf[length + 1];
    memcpy(buf, buffer, length);
    buf[length] = 0;
    ALOGI("Received IR buffer %s\n", buf);
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
