/*
 * Copyright (C) 2018 Amlogic Corporation.
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

#define LOG_TAG "aml_channel_map"


#include <unistd.h>
#include "aml_channel_map.h"
#include "log.h"

struct ch_present {
    char ch_name[16];
    channel_id_t ch_id;
    int  present;
};

struct channel_order {
    channel_order_type_t order_type;
    int ch;
    int ch_mask;  /*not used*/
    channel_info_t channel_info;
};

static const struct channel_order hdmi_pcm8ch = {
    .order_type = CHANNEL_ORDER_HDMIPCM,
    .ch = 8,
    .ch_mask = 0,
    .channel_info.channel_items = {
        {CHANNEL_LEFT_FRONT, 1, 0},
        {CHANNEL_RIGHT_FRONT, 1, 1},
        {CHANNEL_LFE, 1, 2},
        {CHANNEL_CENTER, 1, 3},
        {CHANNEL_LEFT_SURROUND, 1, 4},
        {CHANNEL_RIGHT_SURROUND, 1, 5},
        {CHANNEL_LEFT_DOLBY_ENABLE, 1, 6},
        {CHANNEL_RIGHT_DOLBY_ENABLE, 1, 7},
    }
};

static struct channel_order hdmi_pcm2ch = {
    .order_type = CHANNEL_ORDER_HDMIPCM,
    .ch = 2,
    .ch_mask = 0,
    .channel_info.channel_items = {
        {CHANNEL_LEFT_FRONT, 1, 0},
        {CHANNEL_RIGHT_FRONT, 1, 1},
    },
};

static struct channel_order dolby_2ch = {
    .order_type = CHANNEL_ORDER_DOLBY,
    .ch = 2,
    .ch_mask = 0,
    .channel_info.channel_items = {
        {CHANNEL_LEFT_FRONT, 1, 0},
        {CHANNEL_RIGHT_FRONT, 1, 1},
    },
};

/*currently 8ch is 5.1.2*/
static struct channel_order dolby_8ch = {
    .order_type = CHANNEL_ORDER_DOLBY,
    .ch = 8,
    .ch_mask = 0,
    .channel_info.channel_items = {
        {CHANNEL_LEFT_FRONT, 1, 0},
        {CHANNEL_RIGHT_FRONT, 1, 1},
        {CHANNEL_CENTER, 1, 2},
        {CHANNEL_LFE, 1, 3},
        {CHANNEL_LEFT_SURROUND, 1, 4},
        {CHANNEL_RIGHT_SURROUND, 1, 5},
        {CHANNEL_LEFT_DOLBY_ENABLE, 1, 6},
        {CHANNEL_RIGHT_DOLBY_ENABLE, 1, 7},
    },
};

static struct channel_order dolby_4ch = {
    .order_type = CHANNEL_ORDER_DOLBY,
    .ch = 4,
    .ch_mask = 0,
    .channel_info.channel_items = {
        {CHANNEL_LEFT_FRONT, 1, 0},
        {CHANNEL_RIGHT_FRONT, 1, 1},
        {CHANNEL_CENTER, 1, 2},
        {CHANNEL_LFE, 1, 3},
    },
};

#define CH_ORDER_NUM    10
static struct channel_order ch_orders[CH_ORDER_NUM] = {
};

/*the string is same with Dolby*/
static struct ch_present ch_presents[] = {
    {"lr",  CHANNEL_LEFT_FRONT, 0},
    {"lr",  CHANNEL_RIGHT_FRONT, 0},
    {"c",   CHANNEL_CENTER, 0},
    {"lfe", CHANNEL_LFE, 0},
    {"lrs", CHANNEL_LEFT_SURROUND, 0},
    {"lrs", CHANNEL_RIGHT_SURROUND, 0},
    {"lrtf", CHANNEL_LEFT_TOP_FRONT, 0},
    {"lrtf", CHANNEL_RIGHT_TOP_FRONT, 0},
    {"lrtm", CHANNEL_LEFT_TOP_MIDDLE, 0},
    {"lrtm", CHANNEL_RIGHT_TOP_MIDDLE, 0},
    {"lre",  CHANNEL_LEFT_DOLBY_ENABLE, 0},
    {"lre",  CHANNEL_RIGHT_DOLBY_ENABLE, 0},
};

static void set_channel_info_default()
{
    ch_orders[0] = hdmi_pcm8ch;
    ch_orders[1] = hdmi_pcm2ch;
    ch_orders[2] = dolby_2ch;
    ch_orders[3] = dolby_8ch;
    ch_orders[4] = dolby_4ch;
}

static channel_info_t * get_channel_order(int ch, channel_order_type_t order_type)
{
    int i = 0;
    for (i = 0; i < CH_ORDER_NUM ; i++) {
        if (ch_orders[i].ch == ch && ch_orders[i].order_type == order_type) {
            return &ch_orders[i].channel_info;
        }
    }
    return NULL;
}


static void init_ch_presents(char * speaker_config)
{
    char * token = NULL;
    int item = 0;
    int i = 0;
    char temp[256] = {0};

    if (speaker_config == NULL) {
        return;
    }

    memcpy(temp, speaker_config, strlen(speaker_config));
    item = sizeof(ch_presents) / sizeof(struct ch_present);
    token = strtok(temp, ":");

    for (i = 0; i < item; i++) {
        ch_presents[i].present = 0;
    }
    /*if there is no config, set all the presents on*/
    if (token == NULL) {
        for (i = 0; i < item; i++) {
            ch_presents[i].present = 1;
        }
    }
    while (token != NULL) {

        for (i = 0; i < item; i++) {
            if (!strcmp(ch_presents[i].ch_name, token)) {
                ch_presents[i].present = 1;
                ALOGD(" channel presents=%s id=%d\n", ch_presents[i].ch_name, ch_presents[i].ch_id);
            }
        }
        token = strtok(NULL, ":");

    }
    return;
}

static inline int check_speaker_present(channel_id_t ch_id)
{
    int item = 0;
    int i = 0;
    item = sizeof(ch_presents) / sizeof(struct ch_present);

    for (i = 0; i < item; i++) {
        if (ch_presents[i].ch_id == ch_id) {
            //ALOGE("id=%d presents=%d\n", ch_id, ch_presents[i].present);
            return ch_presents[i].present;
        }
    }
    return 0;
}

static void update_ch_presents(channel_info_t * channel_info)
{
    int i = 0;
    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        channel_info->channel_items[i].present = check_speaker_present(channel_info->channel_items[i].ch_id);
    }

    return;
}


int aml_channelinfo_set(channel_info_t * channel_info, audio_channel_mask_t channel_mask, channel_order_type_t order_type)
{
    int ch = 0;
    int i = 0;
    if (channel_info == NULL) {
        return -1;
    }
    channel_info_t * ch_info = NULL;

    ch = audio_channel_count_from_out_mask(channel_mask);
    memset(channel_info, 0, sizeof(channel_info_t));

    ch_info = get_channel_order(ch, order_type);
    if (ch_info) {
        memcpy(channel_info, ch_info, sizeof(channel_info_t));
    } else {
        for (i = 0; i < ch; i++) {
            channel_info->channel_items[i].ch_id = CHANNEL_BASE + i;
            channel_info->channel_items[i].present = 1;
            channel_info->channel_items[i].order = i;
        }
    }
    return 0;
}





int aml_channelmap_init(aml_channel_map_t ** handle, int ch, char * speaker_config)
{
    int ret = -1;
    int i = 0;
    int size = 1024 * 4;
    void *tmp_map_buffer;
    aml_channel_map_t * channel_map = NULL;
    aml_data_format_t * format = NULL;

    set_channel_info_default();
    ALOGD("MAP INIT\n");

    channel_map = calloc(1 , sizeof(aml_channel_map_t));

    if (channel_map == NULL) {
        ALOGD("channel_map malloc failed\n");
        goto exit;
    }

    tmp_map_buffer = calloc(1 , size);
    if (tmp_map_buffer == NULL) {
        ALOGD("tmp_map_buffer malloc failed\n");
        goto exit;

    }

    channel_map->map_buffer = tmp_map_buffer;
    channel_map->map_buffer_size = size;

    format = &channel_map->format;

    format->ch = ch;

    /*set default value*/
    aml_channelinfo_set(&channel_map->format.channel_info, audio_channel_out_mask_from_count(ch), CHANNEL_ORDER_DOLBY);
    /*get the config */
    init_ch_presents(speaker_config);
    /*update the config*/
    update_ch_presents(&channel_map->format.channel_info);

    *handle = channel_map;

    return 0;
exit:
    if (channel_map) {
        free(channel_map);
    }
    if (tmp_map_buffer) {
        free(tmp_map_buffer);
    }
    return -1;

}

int aml_channelmap_close(aml_channel_map_t * handle)
{
    if (handle == NULL) {
        return 0;
    }
    if (handle->map_buffer) {
        free(handle->map_buffer);
        handle->map_buffer = NULL;
    }

    free(handle);

    return 0;
}

int aml_channelmap_process(aml_channel_map_t * handle, aml_data_format_t *src, void * in_data, size_t nframes)
{
    int i = 0, j = 0;
    int dst_channels = 0;
    int src_channels = 0;
    channel_item_t * dst_channel_item = NULL;
    channel_item_t * dst_channel_item_begin = NULL;

    channel_item_t * src_channel_item = NULL;
    channel_item_t * src_channel_item_begin = NULL;
    aml_data_format_t * dst = NULL;

    size_t need_bytes = 0;
    dst = &handle->format;
    dst->bitwidth = src->bitwidth;
    dst->format   = src->format;
    dst->sr       = src->sr;

    need_bytes = nframes * (dst->bitwidth >> 3) * dst->ch;
    //ALOGE("need=%d map size=%d\n",need_bytes, handle->map_buffer_size);
    if (handle->map_buffer_size < need_bytes) {
        ALOGI("realloc map_buffer_size  from %zu to %zu\n", handle->map_buffer_size, need_bytes);
        handle->map_buffer = realloc(handle->map_buffer, need_bytes);
        if (handle->map_buffer == NULL) {
            ALOGE("realloc map_buffer failed size %zu\n", need_bytes);
            return -1;
        }
        handle->map_buffer_size = need_bytes;
        memset(handle->map_buffer, 0, need_bytes);

    }
    handle->out_buffer_size = need_bytes;

    dst_channel_item = &dst->channel_info.channel_items[0];
    dst_channel_item_begin = &dst->channel_info.channel_items[0];
    while ((void*)dst_channel_item < ((void *)dst_channel_item_begin + sizeof(channel_info_t))) {
        int dst_order = dst_channel_item->order;
        //ALOGE("dst channel=%d present=%d order=%d\n",dst_channel_item->ch_id, dst_channel_item->present,dst_channel_item->order);
        if (dst_channel_item->present) {
            int bfound = 0;
            src_channel_item = &src->channel_info.channel_items[0];
            src_channel_item_begin = &src->channel_info.channel_items[0];

            while ((void*)src_channel_item < ((void *)src_channel_item_begin + sizeof(channel_info_t))) {
                /*src has the same channel*/
                if ((src_channel_item->ch_id == dst_channel_item->ch_id) && src_channel_item->present) {
                    /*copy the src to dst*/
                    int src_order = src_channel_item->order;
                    //ALOGE("src->bitwidth=%d\n",src->bitwidth);
                    if (src->bitwidth == SAMPLE_16BITS) {
                        short * src_data = (short *)in_data;
                        short * dst_data = (short *)handle->map_buffer;
                        int src_channel  = src->ch;
                        int dst_channel  = dst->ch;
                        for (j = 0; j < nframes; j ++) {
                            dst_data[j * dst_channel + dst_order] = src_data[j * src_channel + src_order];

                        }
                    } else if (src->bitwidth == SAMPLE_32BITS) {
                        int * src_data = (int *)in_data;
                        int * dst_data = (int *)handle->map_buffer;
                        int src_channel  = src->ch;
                        int dst_channel  = dst->ch;
                        for (j = 0; j < nframes; j ++) {
                            dst_data[j * dst_channel + dst_order] = src_data[j * src_channel + src_order];
                        }
                    }
                    bfound = 1;
                    break;
                }
                src_channel_item++;
            }

            /*not find the src channel*/
            if (bfound == 0) {
                /*copy the (1+2)/2 of src to this channel*/
                int src_order = src_channel_item->order;
                if (src->bitwidth == SAMPLE_16BITS) {
                    short * src_data = (short *)in_data;
                    short * dst_data = (short *)handle->map_buffer;
                    int src_channel  = src->ch;
                    int dst_channel  = dst->ch;
                    for (j = 0; j < nframes; j ++) {
                        short ch1_data = src_data[j * src_channel + 0];
                        short ch2_data = 0;
                        if (src_channel == 1) {
                            ch2_data = src_data[j * src_channel + 0];
                        } else {
                            ch2_data = src_data[j * src_channel + 1];
                        }

                        dst_data[j * dst_channel + dst_order] = (ch1_data * 0.5 + ch2_data * 0.5);

                    }
                } else if (src->bitwidth == SAMPLE_32BITS) {
                    int * src_data = (int *)in_data;
                    int * dst_data = (int *)handle->map_buffer;
                    int src_channel  = src->ch;
                    int dst_channel  = dst->ch;
                    for (j = 0; j < nframes; j ++) {
                        int ch1_data = src_data[j * src_channel + 0];
                        int ch2_data = 0;
                        if (src_channel == 1) {
                            ch2_data = src_data[j * src_channel + 0];
                        } else {
                            ch2_data = src_data[j * src_channel + 1];
                        }
                        dst_data[j * dst_channel + dst_order] = (ch1_data * 0.5  + ch2_data * 0.5);

                    }

                }
            }
        }
        dst_channel_item++;
    }

    memcpy(&src->channel_info, &dst->channel_info, sizeof(channel_info_t));
    //ALOGE("exit\n");
    return 0 ;
}



