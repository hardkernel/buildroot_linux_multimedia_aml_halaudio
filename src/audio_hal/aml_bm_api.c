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

#define LOG_TAG "aml_audio_bm"
#include <string.h>

#include "log.h"
#include "aml_audio_stream.h"
#include "aml_bm_api.h"
#include "lib_bassmanagement.h"
#ifndef ANDROID
#include "aml_audio_log.h"

static struct ch_name_pair ch_coef_pair[ ] = {
    {"lf_ch_coef",  CHANNEL_LEFT_FRONT},
    {"rf_ch_coef",  CHANNEL_RIGHT_FRONT},
    {"c_ch_coef",   CHANNEL_CENTER},
    {"lfe_ch_coef", CHANNEL_LFE},
    {"ls_ch_coef",  CHANNEL_LEFT_SURROUND},
    {"rs_ch_coef",  CHANNEL_RIGHT_SURROUND},
    {"ltf_ch_coef", CHANNEL_LEFT_TOP_FRONT},
    {"rtf_ch_coef", CHANNEL_RIGHT_TOP_FRONT},
    {"ltm_ch_coef", CHANNEL_LEFT_TOP_MIDDLE},
    {"rtm_ch_coef", CHANNEL_RIGHT_TOP_MIDDLE},
    {"le_ch_coef",  CHANNEL_LEFT_DOLBY_ENABLE},
    {"re_ch_coef",  CHANNEL_RIGHT_DOLBY_ENABLE},
};

static void set_channel_coef(ch_coef_info_t *ch_coef_info, channel_id_t ch, float coef)
{
    int i = 0;
    if (coef > 1.0) {
        coef = 1.0;
    }

    if (coef < 0.0) {
        coef = 0.0;
    }

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if (ch_coef_info->coef_item[i].ch_id == ch) {
            ch_coef_info->coef_item[i].coef = coef;
            break;
        }
    }
    return;
}

static void set_coef_table(ch_coef_info_t *ch_coef_info, channel_info_t * channel_info, float * coef_table)
{
    int i = 0, j = 0;

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        for (j = 0; j < AML_MAX_CHANNELS; j++) {
            //ALOGI("coef ch_id=%d channel id=%d\n",ch_coef_info->coef_item[i].ch_id,channel_info->channel_items[j].ch_id);
            if (ch_coef_info->coef_item[i].ch_id == channel_info->channel_items[j].ch_id) {
                //ALOGI("id=%d present=%d order=%d\n",channel_info->channel_items[j].ch_id,channel_info->channel_items[j].present, channel_info->channel_items[j].order);
                if (channel_info->channel_items[j].present && (channel_info->channel_items[j].order != -1)) {
                    coef_table[channel_info->channel_items[j].order] = ch_coef_info->coef_item[i].coef;
                }
            }
        }
    }


}

static void aml_bm_dumpinfo(void * private)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) private;
    ch_coef_info_t *ch_coef_info = &adev->ch_coef_info;
    int item = 0;
    int i = 0, j = 0;
    item = sizeof(ch_coef_pair) / sizeof(struct ch_name_pair);

    for (i = 0; i < item; i++) {
        for (j = 0; j < AML_MAX_CHANNELS; j++) {
            if (ch_coef_pair[i].ch_id == ch_coef_info->coef_item[i].ch_id) {
                ALOGA("ch name=%s bm coef=%f\n", ch_coef_pair[i].name, ch_coef_info->coef_item[i].coef);
                break;
            }
        }
    }
}


int aml_bm_init(struct aml_audio_device *adev, int val)
{
    int bm_init_param = 0;
    int i = 0;

    if (!adev) {
        ALOGE("adev %p", adev);
        return 1;
    }

    adev->lowerpass_corner = val;

    if (adev->lowerpass_corner == 0) {
        adev->bm_enable = false;
    } else {
        if (adev->lowerpass_corner < BM_MIN_CORNER_FREQUENCY) {
            bm_init_param = 0;
        } else if (adev->lowerpass_corner > BM_MAX_CORNER_FREQUENCY) {
            bm_init_param = (BM_MAX_CORNER_FREQUENCY - BM_MIN_CORNER_FREQUENCY) / BM_STEP_LEN_CORNER_FREQ;
        } else {
            bm_init_param = (adev->lowerpass_corner - BM_MIN_CORNER_FREQUENCY) / BM_STEP_LEN_CORNER_FREQ;
        }
        adev->bm_enable = !aml_bass_management_init(bm_init_param);
    }
    if (adev->bm_init == 0) {
        ch_coef_info_t *ch_coef_info = ch_coef_info = &adev->ch_coef_info;;
        for (i = 0; i < AML_MAX_CHANNELS; i++) {
            ch_coef_info->coef_item[i].ch_id = CHANNEL_BASE + i;
            ch_coef_info->coef_item[i].coef  = 1.0;;
        }
        adev->bm_init = 1;
        aml_log_dumpinfo_install(LOG_TAG, aml_bm_dumpinfo, adev);
    }
    ALOGE("lowerpass_corner %d HZ bm_enable %d init params %d\n",
          adev->lowerpass_corner, adev->bm_enable, bm_init_param);

    return 0;
}


int aml_bm_setparameters(struct audio_hw_device *dev, struct str_parms *parms)
{
    float coef = 1.0;
    int ret = 0;
    int i = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    ch_coef_info_t *ch_coef_info = NULL;

    int item = sizeof(ch_coef_pair) / sizeof(struct ch_name_pair);


    if (!adev || !parms) {
        ALOGE("Fatal Error adev %p parms %p", adev, parms);
        return -1;
    }

    ch_coef_info = &adev->ch_coef_info;

    for (i = 0; i < item; i++) {
        ret = str_parms_get_float(parms, ch_coef_pair[i].name, &coef);
        if (ret >= 0) {
            ALOGI("set ch=%s volume=%f\n", ch_coef_pair[i].name, coef);
            set_channel_coef(ch_coef_info, ch_coef_pair[i].ch_id, coef);
            return 0;
        }
    }

    return -1;
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
    int ret = 0;
    float coef_table[AML_MAX_CHANNELS] = {0.0};
    int lef_index = -1;
    ch_coef_info_t *ch_coef_info = NULL;
    if (!stream || !buffer || !data_format || (bytes == 0)) {
        ALOGE("stream %p buffer %p data_format %p bytes %d");
        return 1;
    }

    adev = aml_out->dev;
    sample_num = bytes/(data_format->ch * (data_format->bitwidth >> 3));

    ch_coef_info = &adev->ch_coef_info;

    if (dump_bm && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
        FILE *fp_original=fopen(DATMOS_ORIGINAL_LFE,"a+");
        fwrite(buffer, 1, bytes,fp_original);
        fclose(fp_original);
    }
    set_coef_table(ch_coef_info, &data_format->channel_info, coef_table);

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if ((data_format->channel_info.channel_items[i].ch_id == CHANNEL_LFE)
            && data_format->channel_info.channel_items[i].present) {
            lef_index = data_format->channel_info.channel_items[i].order;
        }

    }
    //ALOGI("coef table=%f %f %f %f lef_index=%d\n",coef_table[0],coef_table[1],coef_table[2],coef_table[3],lef_index);
    /* due to performance issue, we disable 192K&176K BM process */
    if ((adev->bm_enable) && (data_format->bitwidth == SAMPLE_32BITS) && (data_format->ch == 8) && (lef_index != -1)) {
        ret = aml_bm_lowerpass_process(buffer, bytes, sample_num, data_format->ch, lef_index, coef_table, data_format->bitwidth);
    }

    if (dump_bm && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
        FILE *fp_bm=fopen(DATMOS_BASSMANAGEMENT_LFE,"a+");
        fwrite(buffer, 1, bytes,fp_bm);
        fclose(fp_bm);
    }

    return ret;
}
#else
int aml_bm_init(struct aml_audio_device *adev, int val)
{   
    
    if (!adev) {
        ALOGE("adev %p", adev);
        return 1;
    }
    
    adev->bm_enable = 0;

    return 0;
}
int aml_bm_process(struct audio_stream_out *stream
                    , const void *buffer
                    , size_t bytes
                    , aml_data_format_t *data_format)
{
    return 0;
}

#endif
