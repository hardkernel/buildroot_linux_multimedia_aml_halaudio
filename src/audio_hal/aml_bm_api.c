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

#define LOG_TAG "aml_bm_api"
#include <string.h>

#include "log.h"
#include "aml_audio_stream.h"
#include "aml_bm_api.h"
#include "lib_bassmanagement.h"

int aml_bm_init(struct aml_audio_device *adev, int val)
{
    int bm_init_param = 0;

    if (!adev) {
        ALOGE("adev %p", adev);
        return 1;
    }

    adev->bm_corner_freq = val;

    if (adev->bm_corner_freq == 0)
        adev->bm_enable = false;
    else {
        if (adev->bm_corner_freq < BM_MIN_CORNER_FREQUENCY)
            bm_init_param = 0;
        else if (adev->bm_corner_freq > BM_MAX_CORNER_FREQUENCY)
            bm_init_param = (BM_MAX_CORNER_FREQUENCY - BM_MIN_CORNER_FREQUENCY)/BM_STEP_LEN_CORNER_FREQ;
        else
            bm_init_param = (adev->bm_corner_freq - BM_MIN_CORNER_FREQUENCY)/BM_STEP_LEN_CORNER_FREQ;
        adev->bm_enable = !aml_bass_management_init(bm_init_param);
    }
    ALOGE("bm_corner_freq %d HZ bm_enable %d init params %d\n",
        adev->bm_corner_freq, adev->bm_enable, bm_init_param);

    return 0;
}


int aml_bm_process(struct audio_stream_out *stream
                    , const void *buffer
                    , size_t bytes
                    , aml_data_format_t *data_format)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = NULL;
    /*this is bass management part!*/
    int dump_bm = 0;
    int i = 0;
    int j = 0;
    int32_t *sample = (int32_t *)buffer;
    int sample_num =  0;
    int lowpass = 0;

    if (!stream || !buffer || !data_format || (bytes == 0)) {
        ALOGE("stream %p buffer %p data_format %p bytes %d");
        return 1;
    }

    adev = aml_out->dev;
    sample_num = bytes/(data_format->ch * (data_format->bitwidth >> 3));

    if (dump_bm && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
        FILE *fp_original=fopen(DATMOS_ORIGINAL_LFE,"a+");
        fwrite(buffer, 1, bytes,fp_original);
        fclose(fp_original);
    }

    if ((adev->bm_enable) && (data_format->bitwidth == SAMPLE_32BITS) && (data_format->ch == 8)) {
        for (i = 0; i < sample_num; i++) {
            lowpass = 0;
            for (j = 0; j < data_format->ch; j++) {
                if (j != LFE_CH_INDEX)
                    lowpass += aml_bass_management_process(sample[data_format->ch*i + j], j);
            }

            sample[data_format->ch*i + LFE_CH_INDEX] += lowpass;
        }
    }

    if (dump_bm && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
        FILE *fp_bm=fopen(DATMOS_BASSMANAGEMENT_LFE,"a+");
        fwrite(buffer, 1, bytes,fp_bm);
        fclose(fp_bm);
    }

    return 0;
}


