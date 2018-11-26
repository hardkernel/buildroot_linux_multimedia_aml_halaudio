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

#define LOG_TAG "aml_audio_dca_dec"

#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include "log.h"

#include "audio_hw_utils.h"
#include "aml_dec_api.h"
#include "aml_dca_dec_api.h"
#include "aml_dcv_dec_api.h"
#include "aml_datmos_api.h"


#ifdef USE_AUDIOSERVICE
    static int gdolby_strategy = AML_DOLBY_DECODER;
#else
    static int gdolby_strategy = AML_DOLBY_ATMOS;
#endif


static aml_dec_func_t * get_decoder_function(audio_format_t format, int dolby_strategy)
{
    switch (format) {
    case AUDIO_FORMAT_AC3:
    case AUDIO_FORMAT_E_AC3: {
        if (dolby_strategy == AML_DOLBY_DECODER)
            return &aml_dcv_func;
        else if (dolby_strategy == AML_DOLBY_ATMOS)
            return &aml_datmos_func;
    }
    case AUDIO_FORMAT_DOLBY_TRUEHD:
        return &aml_datmos_func;
    case AUDIO_FORMAT_DTS:
    case AUDIO_FORMAT_DTS_HD: {
        return &aml_dca_func;
    }
    default:
        return NULL;
    }

    return NULL;
}


int aml_decoder_init(aml_dec_t **ppaml_dec, audio_format_t format, aml_dec_config_t * dec_config)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    dec_fun = get_decoder_function(format, gdolby_strategy);
    aml_dec_t *aml_dec_handel = NULL;
    if (dec_fun == NULL) {
        return -1;
    }

    ALOGD("dec_fun->f_init=%p\n", dec_fun->f_init);
    if (dec_fun->f_init) {
        ret = dec_fun->f_init(ppaml_dec, format, dec_config);
    } else {
        return -1;
    }
    aml_dec_handel = *ppaml_dec;
    if (aml_dec_handel) {
        aml_dec_handel->dec_info.output_sr = 48000;
        aml_dec_handel->dec_info.output_ch = 2;
        aml_dec_handel->dec_info.output_bitwidth = SAMPLE_16BITS;
    }
    aml_dec_handel->format = format;

    return ret;


}
int aml_decoder_release(aml_dec_t *aml_dec)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }

    dec_fun = get_decoder_function(aml_dec->format, gdolby_strategy);
    if (dec_fun == NULL) {
        return -1;
    }

    if (dec_fun->f_release) {
        dec_fun->f_release(aml_dec);
    } else {
        return -1;
    }

    return ret;


}
int aml_decoder_config(aml_dec_t *aml_dec, aml_dec_config_t * config)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }
    dec_fun = get_decoder_function(aml_dec->format, gdolby_strategy);
    if (dec_fun == NULL) {
        return -1;
    }

    return ret;
}


int aml_decoder_process(aml_dec_t *aml_dec, unsigned char*buffer, int bytes, int * used_bytes)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    int fill_bytes = 0;
    int parser_raw = 0;

    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }

    dec_fun = get_decoder_function(aml_dec->format, gdolby_strategy);
    if (dec_fun == NULL) {
        return -1;
    }

    fill_bytes = fill_in_the_remaining_data(buffer, bytes, &(aml_dec->raw_deficiency), aml_dec->inbuf, &aml_dec->inbuf_wt, aml_dec->inbuf_max_len);

    /*
     *if fill_bytes = -1
     *then one iec61937 data is completed
     *so, send the raw data to decoder and then call decode_IEC61937_to_raw_data
     */
    //new data in buffer not through parser, and raw data is whole of one iec61937.
    if ((fill_bytes >= 0) && (bytes - fill_bytes >= 0) && (aml_dec->raw_deficiency == 0)) {
        parser_raw = decode_IEC61937_to_raw_data(buffer + fill_bytes
            , bytes - fill_bytes
            , aml_dec->inbuf
            , &aml_dec->inbuf_wt
            , aml_dec->inbuf_max_len
            , &(aml_dec->raw_deficiency)
            , &(aml_dec->IEC61937_raw_size));
    }

    if (dec_fun->f_process) {
        ret = dec_fun->f_process(aml_dec, buffer, bytes);
    } else {
        return -1;
    }

    // change it later
    *used_bytes = bytes;

    return ret;

}


