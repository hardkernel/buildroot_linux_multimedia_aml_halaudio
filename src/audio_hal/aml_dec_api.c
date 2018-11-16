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

static aml_dec_func_t * get_decoder_function(audio_format_t format)
{
    switch (format) {
    case AUDIO_FORMAT_AC3:
    case AUDIO_FORMAT_E_AC3: {
        return &aml_dcv_func;
    }
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
    dec_fun = get_decoder_function(format);
    aml_dec_t *aml_dec_handel = NULL;
    if (dec_fun == NULL) {
        return -1;
    }


    ALOGD("dec_fun->f_init=%p\n", dec_fun->f_init);
    if (dec_fun->f_init) {
        ret = dec_fun->f_init(ppaml_dec, dec_config);
    } else {
        return -1;
    }
    aml_dec_handel = *ppaml_dec;
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

    dec_fun = get_decoder_function(aml_dec->format);
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
    dec_fun = get_decoder_function(aml_dec->format);
    if (dec_fun == NULL) {
        return -1;
    }


    return ret;


}
int aml_decoder_process(aml_dec_t *aml_dec, unsigned char*buffer, int bytes, int * used_bytes)
{
    int ret = -1;
    aml_dec_func_t *dec_fun = NULL;
    if (aml_dec == NULL) {
        ALOGE("%s aml_dec is NULL\n", __func__);
        return -1;
    }

    dec_fun = get_decoder_function(aml_dec->format);
    if (dec_fun == NULL) {
        return -1;
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


