/*
 * Copyright (C) 2017 Amlogic Corporation.
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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <tinyalsa/asoundlib.h>
#include <unistd.h>

#include "audio_hw.h"
#include "alsa_manager.h"
#include "audio_hw_utils.h"
#include "alsa_device_parser.h"


#include <cjson/cJSON.h>


static int alsa_card = 0;

typedef struct alsa_handle {
    unsigned int card;
    alsa_device_t alsa_device;
    struct pcm_config config;
    struct pcm *pcm;

} alsa_handle_t;



static const struct pcm_config pcm_config_out = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_PLAYBACK_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static const struct pcm_config pcm_config_in = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_CAPTURE_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

/* these name should be same in jason*/
#define HDMI_IN       "HDMI_IN"
#define SPDIF_IN      "SPDIF_IN"
#define LINE_IN       "LINE_IN"
#define SPEAKER_OUT   "Speaker_Out"
#define SPDIF_OUT     "Spdif_out"

// currently we don't know where to save it ,we keep it here
#ifdef USE_AUDIOSERVICE
static unsigned char * alsa_config = "{ \"Card\" : 0,\"SPDIF_IN\" : 4, \"LINE_IN\"  : 2,\"Speaker_Out\" : 2, \"Spdif_out\"   : 4 }";
#else
static unsigned char * alsa_config = "{ \"Card\" : 0,\"HDMI_IN\" : 1, \"SPDIF_IN\" : 4, \"LINE_IN\"  : 0,\"Speaker_Out\" : 2, \"Spdif_out\"   : 4 }";
#endif

typedef struct alsa_pair {
    unsigned char * name;
    audio_devices_t device;
    int alsa_card;
    int alsa_device;
} alsa_pair_t;

static cJSON *alsa_root;

static alsa_pair_t alsapairs[] = {
    {
        .name = HDMI_IN,
        .device = AUDIO_DEVICE_IN_HDMI,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = SPDIF_IN,
        .device = AUDIO_DEVICE_IN_SPDIF,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = LINE_IN,
        .device = AUDIO_DEVICE_IN_LINE,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = SPEAKER_OUT,
        .device = AUDIO_DEVICE_OUT_SPEAKER,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = SPDIF_OUT,
        .device = AUDIO_DEVICE_OUT_SPDIF,
        .alsa_card = 0,
        .alsa_device = 0,
    }
};

void printf_alsa_cJSON(const char *json_name, cJSON *pcj)
{
    char *temp;
    temp = cJSON_Print(pcj);
    ALOGD("%s %s\n", json_name, temp);
    free(temp);
}

cJSON *ALSA_CreateJsonRoot(const char *filename)
{
    FILE *fp;
    int len, ret;
    char *input = NULL;
    cJSON *temp;
    if (NULL == filename) {
        return NULL;
    }
    fp = fopen(filename, "r+");
    if (fp == NULL) {
        ALOGD("cannot open the default json file %s\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    len = (int)ftell(fp);
    ALOGD(" length = %d\n", len);

    fseek(fp, 0, SEEK_SET);

    input = (char *)malloc(len + 10);
    if (input == NULL) {
        ALOGD("Cannot malloc the address size = %d\n", len);
        fclose(fp);
        return NULL;
    }
    ret = fread(input, 1, len, fp);
    // allocate the
    temp = cJSON_Parse(input);

    fclose(fp);
    free(input);

    return temp;
}

void aml_alsa_init()
{
    cJSON *temp;
    alsa_pair_t * alsa_item;
    int i = 0;
    //alsa_root = ALSA_CreateJsonRoot(JASON_FILE);
    alsa_root = cJSON_Parse(alsa_config);
    printf_alsa_cJSON("ALSA Configuration", alsa_root);

    temp = cJSON_GetObjectItem(alsa_root, "Card");

    ALOGD("ALSA Card=%d\n", temp->valueint);

    alsa_card = temp->valueint;

    for (i = 0 ; i < sizeof(alsapairs) / sizeof(alsa_pair_t); i++) {

        temp = cJSON_GetObjectItem(alsa_root, alsapairs[i].name);
        if(temp == NULL) {
            ALOGD("%s is NULL\n",alsapairs[i].name);
            continue;
        }
          
        alsapairs[i].alsa_card = alsa_card;
        alsapairs[i].alsa_device = temp->valueint;
        ALOGD("Device name=%s card=%d device=%d\n", alsapairs[i].name, alsapairs[i].alsa_card, alsapairs[i].alsa_device);

    }
    return;
}


int aml_alsa_getdevice(audio_devices_t device, int *alsa_device)
{
    int ret = -1;
    int i = 0;
    for (i = 0 ; i < sizeof(alsapairs) / sizeof(alsa_pair_t); i++) {
        if (alsapairs[i].device & device) {
            *alsa_device = alsapairs[i].alsa_device;
            ret = 0;
            break;
        }
    }

    return ret;
}

int aml_alsa_output_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config)
{
    int ret = -1;
    struct pcm_config *config = NULL;
    int card = 0;
    int device = 0;
    int port = -1;
    struct pcm *pcm = NULL;
    alsa_handle_t * alsa_handle = NULL;


    alsa_handle = (alsa_handle_t *)calloc(1, sizeof(alsa_handle_t));
    if (alsa_handle == NULL) {
        ALOGE("malloc alsa_handle failed\n");
        return -1;
    }

    config = &alsa_handle->config;

    memcpy(config, &pcm_config_out, sizeof(struct pcm_config));

    config->channels = stream_config->channels;
    config->rate     = stream_config->rate;

    if (config->rate == 0 || config->channels == 0) {

        ALOGE("Invalid sampleate=%d channel=%d\n", config->rate == 0, config->channels);
        goto exit;
    }

    if (stream_config->format == AUDIO_FORMAT_PCM_16_BIT) {
        config->format = PCM_FORMAT_S16_LE;
    } else if (stream_config->format == AUDIO_FORMAT_PCM_32_BIT) {
        config->format = PCM_FORMAT_S32_LE;
    } else {
        config->format = PCM_FORMAT_S16_LE;

    }

    config->start_threshold = config->period_size * PLAYBACK_PERIOD_COUNT;
    config->avail_min = 0;

    card = alsa_card;

    aml_alsa_getdevice(device_config->device, &device);

    if (device < 0) {
        ALOGE("Wrong device ID\n");
        return -1;
    }

    ALOGD("In pcm open ch=%d rate=%d\n",config->channels,config->rate);
    ALOGI("%s, audio open card(%d), device(%d) \n", __func__, card, device);
    ALOGI("ALSA open configs: channels %d format %d period_count %d period_size %d rate %d \n",
        config->channels, config->format, config->period_count, config->period_size, config->rate);
    ALOGI("ALSA open configs: threshold start %u stop %u silence %u silence_size %d avail_min %d \n",
        config->start_threshold, config->stop_threshold, config->silence_threshold, config->silence_size, config->avail_min);
    pcm = pcm_open(card, device, PCM_OUT, config);
    if (!pcm || !pcm_is_ready(pcm)) {
        ALOGE("%s, pcm %p open [ready %d] failed \n", __func__, pcm, pcm_is_ready(pcm));
        goto exit;
    }

    alsa_handle->card = card;
    alsa_handle->alsa_device = device;
    alsa_handle->pcm = pcm;


    *handle = (void*)alsa_handle;

    return 0;

exit:
    if (alsa_handle) {
        free(alsa_handle);
    }
    *handle = NULL;
    return -1;

}


void aml_alsa_output_close(void *handle)
{
    ALOGI("\n+%s() hanlde %p\n", __func__, handle);
    alsa_handle_t * alsa_handle = NULL;
    struct pcm *pcm = NULL;

    alsa_handle = (alsa_handle_t *)handle;

    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return;
    }


    pcm = alsa_handle->pcm;
    pcm_close(pcm);
    free(alsa_handle);

    ALOGI("-%s()\n\n", __func__);
}

size_t aml_alsa_output_write(void *handle, const void *buffer, size_t bytes)
{
    int ret = -1;
    alsa_handle_t * alsa_handle = NULL;
    struct pcm *pcm = NULL;
    alsa_handle = (alsa_handle_t *)handle;
    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return -1;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return -1;
    }

    //ALOGD("handle=%p pcm=%p\n",alsa_handle,alsa_handle->pcm);

    ret = pcm_write(alsa_handle->pcm, buffer, bytes);

    return ret;
}

int aml_alsa_input_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config)
{
    int ret = -1;
    struct pcm_config *config = NULL;
    int card = 0;
    int device = 0;
    int port = -1;
    struct pcm *pcm = NULL;
    alsa_handle_t * alsa_handle = NULL;


    alsa_handle = (alsa_handle_t *)calloc(1, sizeof(alsa_handle_t));
    if (alsa_handle == NULL) {
        ALOGE("malloc alsa_handle failed\n");
        return -1;
    }

    config = &alsa_handle->config;

    memcpy(config, &pcm_config_in, sizeof(struct pcm_config));

    config->channels    = stream_config->channels;
    config->rate        = stream_config->rate;
    //config->period_size = stream_config->framesize;

    if (config->rate == 0 || config->channels == 0) {

        ALOGE("Invalid sampleate=%d channel=%d\n", config->rate, config->channels);
        goto exit;
    }

    if (stream_config->format == AUDIO_FORMAT_PCM_16_BIT) {
        config->format = PCM_FORMAT_S16_LE;
    } else if (stream_config->format == AUDIO_FORMAT_PCM_32_BIT) {
        config->format = PCM_FORMAT_S32_LE;
    } else {
        config->format = PCM_FORMAT_S16_LE;
    }

    card = alsa_card;
    aml_alsa_getdevice(device_config->device, &device);

    if (device < 0) {
        ALOGE("Wrong device ID\n");
        return -1;
    }


    config->start_threshold = config->period_size * config->period_count;
    config->avail_min = 0;

    ALOGD("In device=%d alsa device=%d\n", device_config->device, device);
    ALOGD("%s period size=%d\n", __func__, config->period_size);
    pcm = pcm_open(card, device, PCM_IN, config);
    if (!pcm || !pcm_is_ready(pcm)) {
        ALOGE("%s, pcm %p open [ready %d] failed \n", __func__, pcm, pcm_is_ready(pcm));
        goto exit;
    }

    alsa_handle->card = card;
    alsa_handle->alsa_device = device;
    alsa_handle->pcm = pcm;

    *handle = (void*)alsa_handle;

    return 0;

exit:
    if (alsa_handle) {
        free(alsa_handle);
    }
    *handle = NULL;
    return -1;

}


void aml_alsa_input_close(void *handle)
{
    ALOGI("\n+%s() hanlde %p\n", __func__, handle);
    alsa_handle_t * alsa_handle = NULL;
    struct pcm *pcm = NULL;

    alsa_handle = (alsa_handle_t *)handle;

    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return;
    }


    pcm = alsa_handle->pcm;
    pcm_stop(pcm);
    pcm_close(pcm);
    free(alsa_handle);

    ALOGI("-%s()\n\n", __func__);
}

size_t aml_alsa_input_read(void *handle, void *buffer, size_t bytes)
{
    int ret = -1;
    alsa_handle_t * alsa_handle = NULL;
    struct pcm *pcm = NULL;

    alsa_handle = (alsa_handle_t *)handle;

    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return -1;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return -1;
    }
    //ALOGD("alsa read =%d\n",bytes);
    ret = pcm_read(alsa_handle->pcm, buffer, bytes);

    return ret;
}

