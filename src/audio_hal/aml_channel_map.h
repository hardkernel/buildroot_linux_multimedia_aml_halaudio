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

#ifndef AML_CHANNEL_MAP_H
#define AML_CHANNEL_MAP_H

#include "aml_audio_stream.h"

typedef struct aml_channel_mapping {
    aml_data_format_t format;
    void *map_buffer;
    size_t map_buffer_size;
} aml_channel_map_t;


int aml_channelmap_init(aml_channel_map_t ** handle, int dst_ch);
int aml_channelmap_close(aml_channel_map_t * handle);
int aml_channelmap_process(aml_channel_map_t * handle, aml_data_format_t *src, void * in_data, size_t nframes);

int aml_channelinfo_set(channel_info_t * channel_info, audio_channel_mask_t channel_mask);



#endif

