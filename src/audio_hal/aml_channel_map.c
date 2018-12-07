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

int aml_channelinfo_set(channel_info_t * channel_info, audio_channel_mask_t channel_mask)
{
    int ch = 0;
    int i = 0;
    if (channel_info == NULL) {
        return -1;
    }

    ch = audio_channel_count_from_out_mask(channel_mask);
    memset(channel_info, 0, sizeof(channel_info_t));
    for (i = 0; i < ch; i++) {
        channel_info->channel_items[i].ch_id = CHANNEL_BASE + i;
        channel_info->channel_items[i].present = 1;
        channel_info->channel_items[i].order = i;
    }

    return 0;
}


int aml_channelmap_init(aml_channel_map_t ** handle, int dst_ch)
{
    int ret = -1;
    int i = 0;
    int size = 1024 * 4;
    void *tmp_map_buffer;
    aml_channel_map_t * channel_map = NULL;
    aml_data_format_t * format = NULL;

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

    format->ch = dst_ch;
    aml_channelinfo_set(&channel_map->format.channel_info, audio_channel_out_mask_from_count(dst_ch));

    *handle = channel_map;

    return 0;
exit:
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
    }

    handle->map_buffer_size = need_bytes;


    dst_channel_item = &dst->channel_info.channel_items[0];
    dst_channel_item_begin = &dst->channel_info.channel_items[0];
    while ((void*)dst_channel_item < ((void *)dst_channel_item_begin + sizeof(channel_info_t))) {
        int dst_order = dst_channel_item->order;
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

    memcpy(&src->channel_info, &dst->channel_info,sizeof(channel_info_t));
    //ALOGE("exit\n");
    return 0 ;
}



