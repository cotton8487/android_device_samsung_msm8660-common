/*
 * Copyright (C) 2017, The LineageOS Project
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

#define LOG_TAG "audio_amplifier"
#define LOG_NDEBUG 0

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>

#include <cutils/log.h>

#include <hardware/audio_amplifier.h>

#include <msm8660/platform.h>

#include <linux/a2220.h>

#define A2220_DEVICE "/dev/audience_a2220"

#define UNUSED __attribute__((unused))

typedef struct a2220_dev {
    amplifier_device_t amp_dev;
    audio_mode_t current_mode;
    int fd;
    int mode;
    pthread_mutex_t lock;
} a2220_device_t;

static a2220_device_t *a2220_dev = NULL;

int a2220_init(a2220_device_t *a2220_dev)
{
    int rc = -1;

    a2220_dev->fd = open(A2220_DEVICE, O_RDWR);
    a2220_dev->mode = A2220_PATH_SUSPEND;

    if (!a2220_dev->fd) {
        ALOGE("%s: unable to open a2220 device!", __func__);
        close(a2220_dev->fd);
        return rc;
    } else {
        ALOGV("%s: device opened, fd=%d", __func__, a2220_dev->fd);
        pthread_mutex_init(&a2220_dev->lock, NULL);
    }

    return 0;
}

int a2220_set_mode(a2220_device_t *a2220_dev, int mode)
{
    int rc = -1;

    if (a2220_dev->mode != mode) {
        pthread_mutex_lock(&a2220_dev->lock);

        rc = ioctl(a2220_dev->fd, A2220_SET_CONFIG, mode);
        if (rc < 0) {
            ALOGE("%s: ioctl failed, errno=%d", __func__, errno);
        } else {
            a2220_dev->mode = mode;
            ALOGV("%s: Audience A2220 mode is set to %d.", __func__, mode);
        }
        pthread_mutex_unlock(&a2220_dev->lock);
    }
    return rc;
}

static int amp_set_mode(amplifier_device_t *device, audio_mode_t mode)
{
    int ret = 0;
    a2220_device_t *dev = (a2220_device_t *) device;

    dev->current_mode = mode;

    return ret;
}

static int amp_enable_input_devices(amplifier_device_t *device,
        uint32_t devices, bool enable)
{
    a2220_device_t *dev = (a2220_device_t *) device;

    int mode = A2220_PATH_SUSPEND;

    if (dev->current_mode == AUDIO_MODE_IN_CALL || dev->current_mode == AUDIO_MODE_IN_COMMUNICATION) {
        /* Enable noise suppression for input */
        switch (devices) {
            case SND_DEVICE_IN_VOIP_HANDSET_MIC:
                mode = A2220_PATH_INCALL_RECEIVER_NSON;
                break;
            case SND_DEVICE_IN_SPEAKER_DMIC:
            case SND_DEVICE_IN_SPEAKER_DMIC_AEC:
            case SND_DEVICE_IN_SPEAKER_DMIC_NS:
            case SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS:
            case SND_DEVICE_IN_VOIP_SPEAKER_MIC:
                if (enable)
                    mode = A2220_PATH_INCALL_SPEAKER;
            case SND_DEVICE_IN_VOIP_HEADSET_MIC:
                if (enable)
                    mode = A2220_PATH_INCALL_HEADSET;
                break;
            default:
                if (enable)
                    mode = A2220_PATH_SUSPEND;
                break;
        }
    }

    a2220_set_mode(dev, mode);

    return 0;
}

static int amp_dev_close(hw_device_t *device)
{
    a2220_device_t *dev = (a2220_device_t *) device;

    if (dev->fd >= 0)
        close(dev->fd);

    pthread_mutex_destroy(&a2220_dev->lock);

    free(dev);

    return 0;
}

static int amp_module_open(const hw_module_t *module, UNUSED const char *name,
        hw_device_t **device)
{
    if (a2220_dev) {
        ALOGE("%s:%d: Unable to open second instance of A2220 amplifier\n",
                __func__, __LINE__);
        return -EBUSY;
    }

    a2220_dev = calloc(1, sizeof(a2220_device_t));
    if (!a2220_dev) {
        ALOGE("%s:%d: Unable to allocate memory for amplifier device\n",
                __func__, __LINE__);
        return -ENOMEM;
    }

    a2220_dev->amp_dev.common.tag = HARDWARE_DEVICE_TAG;
    a2220_dev->amp_dev.common.module = (hw_module_t *) module;
    a2220_dev->amp_dev.common.version = HARDWARE_DEVICE_API_VERSION(1, 0);
    a2220_dev->amp_dev.common.close = amp_dev_close;

    a2220_dev->amp_dev.set_input_devices = NULL;
    a2220_dev->amp_dev.set_output_devices = NULL;
    a2220_dev->amp_dev.enable_input_devices = amp_enable_input_devices;
    a2220_dev->amp_dev.enable_output_devices = NULL;
    a2220_dev->amp_dev.set_mode = amp_set_mode;
    a2220_dev->amp_dev.output_stream_start = NULL;
    a2220_dev->amp_dev.input_stream_start = NULL;
    a2220_dev->amp_dev.output_stream_standby = NULL;
    a2220_dev->amp_dev.input_stream_standby = NULL;

    a2220_dev->current_mode = AUDIO_MODE_NORMAL;

    a2220_init(a2220_dev);

    *device = (hw_device_t *) a2220_dev;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = amp_module_open,
};

amplifier_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AMPLIFIER_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AMPLIFIER_HARDWARE_MODULE_ID,
        .name = "MSM8660 audio amplifier HAL",
        .author = "The LineageOS Project",
        .methods = &hal_module_methods,
    },
};
