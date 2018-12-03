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

#define LOG_TAG "aml_datmos_api"

#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>
#include <cutils/str_parms.h>
#include <dlfcn.h>

#include "log.h"
#include "aml_datmos_api.h"
#include "audio_hw.h"
#include "aml_datmos_config.h"
#include "audio_core.h"

/*marco define*/
#define VAL_LEN 256
#define MAX_DECODER_MAT_FRAME_LENGTH 61424
#define MAX_DECODER_DDP_FRAME_LENGTH 0X1800
#define MAX_DECODER_THD_FRAME_LENGTH 8190


#define DATMOS_HT_OK                  0
#define DATMOS_HT_ENOMEM              1
#define DATMOS_HT_EINVAL_PARAM        2
#define DATMOS_HT_EINVAL_CONFIG       3
#define DATMOS_HT_EPROC_GENERAL       4
#define DATMOS_HT_EPROC_STREAM        5
#define DATMOS_HT_EPROC_NOT_PREROLLED 6

#define ONE_BLOCK_FRAME_SIZE 256
#define MAX_BLOCK_NUM 32
#define AUDIO_FORMAT_STRING(format) ((format) == TRUEHD) ? ("TRUEHD") : (((format) == AC3) ? ("AC3") : (((format) == EAC3) ? ("EAC3") : ("LPCM")))

#define DATMOS_PCM_OUT_FILE "/tmp/datmos_pcm_out.data"

#define DATMOS_RAW_IN_FILE "/tmp/datmos_raw_in.data"
/*global parameters*/

/*static parameters*/
typedef enum {
    DAP_Reserved = 0x00,
    DAP_Standard,
    DAP_Movie,
    DAP_Music,
    DAP_Night,
} GUI_DAP_VALUE;

/*basic fuction*/
static int convert_format (unsigned char *buffer, int size){
    int idx;
    unsigned char tmp;

    for (idx = 0; idx < size;)
    {
        //buffer[idx] = (unsigned short)BYTE_REV(buffer[idx]);
        tmp = buffer[idx];
        buffer[idx] = buffer[idx + 1];
        buffer[idx + 1] = tmp;
        idx = idx + 2;
    }
    return size;
}
/*datmos api*/
#ifdef DATMOS
int datmos_set_parameters(struct audio_hw_device *dev, struct str_parms *parms)
{
    char value[VAL_LEN];
    int val = 0;
    int ret = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    void *opts = NULL;
    if (adev->datmos_enable)
        opts = get_datmos_current_options();

    if (!adev || !parms) {
        ALOGE("Fatal Error adev %p parms %p", adev, parms);
        goto error_exit;
    }

    ret = str_parms_get_str(parms, "speakers", value, sizeof(value));
    if (ret >= 0) {
        memset(adev->datmos_param.speaker_config, 0, sizeof(adev->datmos_param.speaker_config));
        strncpy(adev->datmos_param.speaker_config, value, strlen(value));
        ALOGI("speaker_config set to %s\n", adev->datmos_param.speaker_config);
        /*datmos parameter*/
        if (adev->datmos_enable)
            add_datmos_option(opts, "-speakers", adev->datmos_param.speaker_config);
        return 0;
    }

    ret = str_parms_get_str(parms, "directdec", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        if (strncmp(value, "enable", 6) == 0 || strncmp(value, "1", 1) == 0) {
            adev->datmos_param.directdec = 1;
        } else if (strncmp(value, "disable", 7) == 0 || strncmp(value, "0", 1) == 0) {
            adev->datmos_param.directdec = 0;
        } else {
            ALOGE("%s() unsupport directdec value: %s",   __func__, value);
        }
        ALOGI("directdec set to %d\n", adev->datmos_param.directdec);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.directdec) {
                char directdec_config[12];
                sprintf(directdec_config, "%d", adev->datmos_param.directdec);
                add_datmos_option(opts, "-directdec", "");
            }
            else
                delete_datmos_option(opts, "-directdec");
        }
        return 0;
    }

    ret = str_parms_get_str(parms, "drc", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        memset(adev->datmos_param.drc_config, 0, sizeof(adev->datmos_param.drc_config));
        strncpy(adev->datmos_param.drc_config, value, strlen(value));
        ALOGI("drc_config set to %s\n", adev->datmos_param.drc_config);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            add_datmos_option(opts, "-drc", adev->datmos_param.drc_config);
        }
        return 0;
    }

    ret = str_parms_get_str(parms, "virt", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        memset(adev->datmos_param.virt_config, 0, sizeof(adev->datmos_param.virt_config));
        strncpy(adev->datmos_param.virt_config, value, strlen(value));
        ALOGI("virt_config set to %s\n", adev->datmos_param.virt_config);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            add_datmos_option(opts, "-virt", adev->datmos_param.virt_config);
        }
        return 0;
    }

    ret = str_parms_get_int(parms, "post", &val);
    if (ret >= 0) {
        adev->datmos_param.post = val;
        ALOGI("post set to %d\n", adev->datmos_param.post);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.post)
                add_datmos_option(opts, "-post", "");
            else
                delete_datmos_option(opts, "-post");
        }
        return 0;
    }

    ret = str_parms_get_int(parms, "mode", &val);
    if (ret >= 0) {
        adev->datmos_param.mode = val;
        ALOGI("mode set to %d\n", adev->datmos_param.mode);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            const char *opt_key = "-mode";
            switch (adev->datmos_param.mode) {
                case DAP_Standard:
                    /*use the default value, "-mode movie"*/
                    delete_datmos_option(opts, opt_key);
                    break;
                case DAP_Movie:
                    add_datmos_option(opts, opt_key, "movie");
                    break;
                case DAP_Music:
                    add_datmos_option(opts, opt_key, "music");
                    break;
                case DAP_Night:
                    add_datmos_option(opts, opt_key, "night");
                    break;
                default:
                    ALOGI("wrong mode %d\n", adev->datmos_param.mode);
            }
        }
        return 0;
    }

    ret = str_parms_get_int(parms, "vmcal", &val);
    if (ret >= 0) {
        adev->datmos_param.vmcal = val;
        ALOGI("vmcal set to %d\n", adev->datmos_param.vmcal);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.vmcal) {
                char vmcal_config[12];
                sprintf(vmcal_config, "%d", adev->datmos_param.vmcal);
                add_datmos_option(opts, "-vmcal", vmcal_config);
            }
            else
                delete_datmos_option(opts, "-vmcal");
        }
        return 0;
    }

    ret = str_parms_get_int(parms, "hfilt", &val);
    if (ret >= 0) {
        adev->datmos_param.hfilt = val;
        ALOGI("hfilt set to %d\n", adev->datmos_param.hfilt);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.hfilt)
                add_datmos_option(opts, "-hfilt", "");
            else
                delete_datmos_option(opts, "-hfilt");
        }
        return 0;
    }

    ret = str_parms_get_int(parms, "nolm", &val);
    if (ret >= 0) {
        adev->datmos_param.nolm = val;
        ALOGI("nolm set to %d\n", adev->datmos_param.nolm);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.nolm)
                add_datmos_option(opts, "-nolm", "");
            else
                delete_datmos_option(opts, "-nolm");
        }
        return 0;
    }

    ret = str_parms_get_int(parms, "noupmix", &val);
    if (ret >= 0) {
        adev->datmos_param.noupmix = val;
        ALOGI("noupmix set to %d\n", adev->datmos_param.noupmix);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.noupmix)
                add_datmos_option(opts, "-noupmix", "");
            else
                delete_datmos_option(opts, "-noupmix");
        }
        return 0;
    }

    ret = str_parms_get_int(parms, "novlamp", &val);
    if (ret >= 0) {
        adev->datmos_param.novlamp = val;
        ALOGI("novlamp set to %d\n", adev->datmos_param.novlamp);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.novlamp)
                add_datmos_option(opts, "-novlamp", "");
            else
                delete_datmos_option(opts, "-novlamp");
        }
        return 0;
    }

    ret = str_parms_get_str(parms, "verbose", value, sizeof(value));
    if (ret >= 0) {
        ALOGI("get value %s\n", value);
        if (strncmp(value, "enable", 6) == 0 || strncmp(value, "1", 1) == 0) {
            adev->datmos_param.verbose = 1;
        } else if (strncmp(value, "disable", 7) == 0 || strncmp(value, "0", 1) == 0) {
            adev->datmos_param.verbose = 0;
        } else {
            ALOGE("%s() unsupport directdec value: %s",   __func__, value);
        }
        ALOGI("verbose set to %d\n", adev->datmos_param.verbose);
        /*datmos parameter*/
        if (adev->datmos_enable) {
            if (adev->datmos_param.verbose){
                ALOGI("add_datmos_option -v\n");
                add_datmos_option(opts, "-v", "");
            }
            else
                delete_datmos_option(opts, "-v");
        }
        return 0;
    }

    return -1;

error_exit:
    ret = -1;
    ALOGE("Error exit!");
    return ret;
}

int datmos_get_parameters(struct audio_hw_device *dev, const char *keys, char *temp_buf, size_t temp_buf_size)
{
    return 1;
}


int get_datmos_func(struct aml_datmos_param *datmos_handle)
{
    ALOGI("");

    if (!datmos_handle) {
        ALOGE("datmos_handle is NULL");
        return 1;
    }

    if (!datmos_handle->fd) {
        datmos_handle->fd = dlopen("/tmp/ds/0xc7_0xfba4_0x5e.so", RTLD_LAZY);
        if (datmos_handle->fd == NULL) {
            ALOGI("error dlopen %s", dlerror());
            return 1;
        }

        datmos_handle->get_audio_info = (int (*)(void *, int *))dlsym(datmos_handle->fd, "get_audio_info");
        if (datmos_handle->get_audio_info == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_atmos_init = (int (*)(unsigned int, void **, bool, const int, const char **))dlsym(datmos_handle->fd, "aml_atmos_init");
        if (datmos_handle->aml_atmos_init == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_atmos_process = (int (*)(void *, unsigned int, void *, size_t *, char *, unsigned int , size_t *))dlsym(datmos_handle->fd, "aml_atmos_process");
        if (datmos_handle->aml_atmos_process == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_atmos_cleanup = (void (*)(void *))dlsym(datmos_handle->fd, "aml_atmos_cleanup");
        if (datmos_handle->aml_atmos_cleanup == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        datmos_handle->aml_get_output_info = (int (*)(void *, int *, int *, int *))dlsym(datmos_handle->fd, "aml_get_output_info");
        if (datmos_handle->aml_get_output_info == NULL) {
            ALOGI("cant find lib interface %s", dlerror());
            goto error;
        }
        ALOGI("fd=%p", datmos_handle->fd);
        ALOGI("get_audio_info=%p", datmos_handle->get_audio_info);
        ALOGI("aml_atmos_init=%p", datmos_handle->aml_atmos_init);
        ALOGI("aml_atmos_process=%p", datmos_handle->aml_atmos_process);
        ALOGI("aml_atmos_cleanup=%p", datmos_handle->aml_atmos_cleanup);
        ALOGI("aml_get_output_info=%p", datmos_handle->aml_get_output_info);
        return 0;
    } else {
        ALOGI("");
        return 0;
    }

error:
    ALOGE("dlopen failed!");
    dlclose(datmos_handle->fd);
    datmos_handle->fd = NULL;
    return 1;
}

int cleanup_atmos_func(struct aml_datmos_param *datmos_handle)
{
    ALOGI("");
    if (!datmos_handle) {
        ALOGE("datmos_handle is NULL");
        return 1;
    }
    datmos_handle->get_audio_info = NULL;
    datmos_handle->aml_atmos_init = NULL;
    datmos_handle->aml_atmos_process = NULL;
    datmos_handle->aml_atmos_cleanup = NULL;
    datmos_handle->aml_get_output_info = NULL;

    /* close the shared object */
    if (datmos_handle->fd) {
        dlclose(datmos_handle->fd);
        datmos_handle->fd = NULL;
        ALOGE("fd is dlclosed!");
    }
    return 0;
}


static void mat_frame_debug(const char *mat_frame, int raw_size)
{
    int i = 0;
    if (mat_frame) {
        for (i = 0; i < 16; i++) {
            printf("%2x ", mat_frame[i]);
        }
        printf("\n");
        for (i = 0; i < 16; i++) {
            printf("%2x ", mat_frame[raw_size - 16 + i]);
        }
        printf("\n");
    }
}

static int datmos_get_output_info(aml_dec_t *aml_dec, aml_dec_info_t *dec_info)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    int ret = 0;
    ALOGV("<<IN>>");
    if (!datmos_dec || !dec_info) {
        ALOGE("%s dec_info is NULL\n", __func__);
        return -1;
    }
    else {
        if (datmos_dec->aml_get_output_info) {
            ret = datmos_dec->aml_get_output_info(aml_dec->dec_ptr
                , &dec_info->output_sr
                , &dec_info->output_ch
                , &dec_info->output_bitwidth);
            ALOGV("output_sr %d output_ch %d output_bitwidth %d",
                dec_info->output_sr, dec_info->output_ch, 
                dec_info->output_bitwidth);
        }
        return ret;
    }

}

static int datmos_get_atmos_info(aml_dec_t *aml_dec)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    int ret = 0;

    if (!datmos_dec) {
        ALOGE("%s datmos_dec is NULL\n", __func__);
        return -1;
    }
    else {
        if (datmos_dec->get_audio_info && aml_dec->dec_ptr) {
            ret = datmos_dec->get_audio_info(aml_dec->dec_ptr, &datmos_dec->is_dolby_atmos);
            ALOGV("is_dolby_atmos %d",datmos_dec->is_dolby_atmos);
            ret = 0;
        }
        else
            ret = -1;
        return ret;
    }

}

int datmos_decoder_init_patch(aml_dec_t ** ppdatmos_dec, audio_format_t format, aml_dec_config_t * dec_config)
{
    struct dolby_atmos_dec *datmos_dec;
    aml_dec_t  *aml_dec = NULL;
    int init_argc = 0;
    char **init_argv = NULL;
    struct aml_datmos_param *datmos_handle = NULL;
    void *opts = NULL;

    ALOGV("<<IN>>");
    if (!dec_config) {
        ALOGE("dec_config is error\n");
        return -1;
    }
    aml_datmos_config_t *datmos_config = (aml_datmos_config_t *)dec_config;
    datmos_dec = calloc(1, sizeof(struct dolby_atmos_dec));
    if (datmos_dec == NULL) {
        ALOGE("malloc datmos_dec failed\n");
        return -1;
    }


    if (init_argv == NULL) {
        init_argv = (char **)malloc(MAX_PARAM_COUNT * VALUE_BUF_SIZE);
        if (init_argv) {
            memset(init_argv, 0, sizeof(MAX_PARAM_COUNT * VALUE_BUF_SIZE));
            for (int i = 0; i < MAX_PARAM_COUNT; i++) {
                init_argv[i] = (char *)malloc(VALUE_BUF_SIZE);
                memset(init_argv[i], 0, sizeof(VALUE_BUF_SIZE));
            }
        }
    }

    datmos_config->audio_type = android_audio_format_t_convert_to_andio_type(format);
    /*Fixme: how to get the eb3 extension?*/
    datmos_config->is_eb3_extension = 0;
    datmos_handle = (struct aml_datmos_param *)datmos_config->reserved;
    opts = get_datmos_current_options();

    if (get_datmos_config(opts, init_argv, &init_argc) != 0) {
        ALOGE("get datmos config fail\n");
        return -1;
    }

    aml_dec = &datmos_dec->aml_dec;
    ALOGI("audio_type %s is_eb3_extension %d\n", AUDIO_FORMAT_STRING(datmos_config->audio_type), datmos_config->is_eb3_extension);

    if (datmos_handle->aml_atmos_init)
        aml_dec->status =
            datmos_handle->aml_atmos_init(
                    datmos_config->audio_type
                    , &(aml_dec->dec_ptr)
                    , datmos_config->is_eb3_extension
                    , init_argc
                    , init_argv);

    ALOGI("aml_dec %p status %d format %#x\n", aml_dec, aml_dec->status, aml_dec->format);

    ALOGI("aml_atmos_init return %s\n", ((aml_dec->status == 0) ? "success" : "fail"));

    if (aml_dec->status < 0) {
        goto exit;
    }

    aml_dec->remain_size = 0;
    aml_dec->outlen_pcm = 0;
    aml_dec->outlen_raw = 0;
    aml_dec->inbuf = NULL;
    aml_dec->outbuf = NULL;
    aml_dec->outbuf_raw = NULL;
    if ((datmos_config->audio_type == AC3) || (datmos_config->audio_type == EAC3)) 
        aml_dec->inbuf_max_len = MAX_DECODER_DDP_FRAME_LENGTH;
    else if (datmos_config->audio_type == TRUEHD)
        aml_dec->inbuf_max_len = MAX_DECODER_MAT_FRAME_LENGTH*4;
    ALOGV("aml_dec inbuf_max_len %x\n", aml_dec->inbuf_max_len);
    aml_dec->inbuf = (unsigned char*) malloc(aml_dec->inbuf_max_len);
    /*
     *FIXME:local playback of truehd case is TODO.
    else if (datmos_config->audio_type == TRUEHD)
        aml_dec->inbuf = (unsigned char*) malloc(MAX_DECODER_THD_FRAME_LENGTH);
    */
    if (!aml_dec->inbuf) {
        ALOGE("malloc buffer failed\n");
        goto exit;
    }
    aml_dec->outbuf_max_len = MAX_BLOCK_NUM*ONE_BLOCK_FRAME_SIZE*MAX_OUTPUT_CH*BYTES_PER_SAMPLE;
    ALOGV("aml_dec outbuf_max_len %x\n", aml_dec->outbuf_max_len);
    aml_dec->outbuf = (unsigned char*) malloc(aml_dec->outbuf_max_len);
    if (!aml_dec->outbuf) {
        ALOGE("malloc buffer failed\n");
        goto exit;
    }

    datmos_dec->get_audio_info = datmos_handle->get_audio_info;
    datmos_dec->aml_atmos_init = datmos_handle->aml_atmos_init;
    datmos_dec->aml_atmos_process = datmos_handle->aml_atmos_process;
    datmos_dec->aml_atmos_cleanup = datmos_handle->aml_atmos_cleanup;
    datmos_dec->aml_get_output_info = datmos_handle->aml_get_output_info;
    aml_dec->status = 1;

    *ppdatmos_dec = (aml_dec_t *)datmos_dec;
    if (init_argv) {
        free(init_argv);
        init_argv = NULL;
    }
    ALOGV("<<OUT>>");
    return 1;

exit:

    if (aml_dec->inbuf) {
        free(aml_dec->inbuf);
        aml_dec->inbuf = NULL;
    }
    if (aml_dec->outbuf) {
        free(aml_dec->outbuf);
        aml_dec->outbuf = NULL;
    }
    if (datmos_dec) {
        free(datmos_dec);
        datmos_dec = NULL;
    }
    if (init_argv) {
        free(init_argv);
        init_argv = NULL;
    }
    *ppdatmos_dec = NULL;
    ALOGV("<<OUT>>");
    return -1;

}

int datmos_decoder_release_patch(aml_dec_t *aml_dec)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    ALOGV("<<IN>>");
    if (datmos_dec && datmos_dec->aml_atmos_cleanup) {
        datmos_dec->aml_atmos_cleanup(aml_dec->dec_ptr);
        aml_dec->dec_ptr = NULL;
    }

    if (aml_dec->status == 1) {
        aml_dec->status = 0;
        aml_dec->remain_size = 0;
        aml_dec->outlen_pcm = 0;
        aml_dec->outlen_raw = 0;
        if (aml_dec->inbuf) {
            free(aml_dec->inbuf);
            aml_dec->inbuf = NULL;
        }
        if (aml_dec->outbuf) {
            free(aml_dec->outbuf);
            aml_dec->outbuf = NULL;
        }
        if (aml_dec->outbuf_raw) {
            free(aml_dec->outbuf_raw);
            aml_dec->outbuf_raw = NULL;
        }
        datmos_dec->get_audio_info = NULL;
        datmos_dec->aml_atmos_init = NULL;
        datmos_dec->aml_atmos_process = NULL;
        datmos_dec->aml_atmos_cleanup = NULL;
        datmos_dec->aml_get_output_info = NULL;
        free(aml_dec);
        aml_dec = NULL;
    }
    ALOGV("<<OUT>>");
    return 1;
}

int datmos_decoder_process_patch(aml_dec_t *aml_dec, unsigned char*in_buffer, int in_bytes)
{
    struct dolby_atmos_dec *datmos_dec = (struct dolby_atmos_dec *)aml_dec;
    size_t bytes_consumed = 0;
    size_t pcm_produced_bytes = 0;
    int ret = 0;
    size_t input_threshold = 0;
    size_t dump_input = 0;
    size_t dump_output = 0;
    size_t check_data = 0;

    ALOGV("<<IN>>");
    if (!aml_dec || !in_buffer || (in_bytes <= 0) || !aml_dec->inbuf) {
        ALOGE("aml_dec %p in_buffer %p in_bytes %#x inbuf %p inbuf_wt %#x\n",
            aml_dec, in_buffer, in_bytes, aml_dec->inbuf, aml_dec->inbuf_wt);
        goto EXIT;
    }

    /*FIXME, sometimes the data is not right, decoder could not preroll the datmos at all*/
    input_threshold = aml_dec->burst_payload_size;

    if ((aml_dec->inbuf_wt >= input_threshold) && (input_threshold > 0)) {
        ALOGV("inbuf_wt %#x burst_payload_size %#x, input_threshold %#x\n", aml_dec->inbuf_wt, aml_dec->burst_payload_size, input_threshold);
        datmos_dec->is_truehd_within_mat = is_truehd_within_mat(aml_dec->inbuf, input_threshold);

        /*begin to decode the data*/
        ret = datmos_dec->aml_atmos_process
                        (aml_dec->inbuf
                        , input_threshold
                        , aml_dec->dec_ptr
                        , &bytes_consumed
                        , aml_dec->outbuf
                        , aml_dec->outbuf_max_len
                        , &pcm_produced_bytes);

        if (check_data) {
            mat_frame_debug(aml_dec->inbuf, input_threshold);
            char *datmos_data = aml_dec->inbuf;
            ALOGE("header %2x %2x", datmos_data[0], datmos_data[1]);
        }

        ALOGV("valid bytes %#x outlen_pcm %#x", aml_dec->inbuf_wt, aml_dec->outlen_pcm);
        /*end of the decode*/
        {
            //dump the input data, as the data should convert to LSB
            if (dump_input) {
                FILE *fpa=fopen(DATMOS_RAW_IN_FILE,"a+");
                // convert_format(aml_dec->inbuf, aml_dec->burst_payload_size);
                fwrite((char *)aml_dec->inbuf, 1, aml_dec->burst_payload_size,fpa);
                fclose(fpa);
            }
            //update the inbuf_wt and flush the consumed data in the inbuf
            {
                if (aml_dec->inbuf_wt - aml_dec->burst_payload_size > 0) {
                    memmove(aml_dec->inbuf, aml_dec->inbuf + aml_dec->burst_payload_size, aml_dec->inbuf_wt - aml_dec->burst_payload_size);
                    aml_dec->inbuf_wt -= aml_dec->burst_payload_size;
                }
                else {
                    memset(aml_dec->inbuf, 0, aml_dec->inbuf_wt);
                    aml_dec->inbuf_wt = 0;
                }
            }
            /*
             *for this ret means that datmos would block this status
             *we should return -1, the decoder should be reinit again!
             */
            if ((ret == 4) && (aml_dec->format == AUDIO_FORMAT_DOLBY_TRUEHD))  {
                /*pay more attention here, if flush the inbuf, should reset the raw_deficiency&inbuf_wt to zero*/
                // memset(aml_dec->inbuf, 0, aml_dec->inbuf_max_len);
                // aml_dec->inbuf_wt = 0;
                // aml_dec->raw_deficiency =0;
                aml_dec->outlen_pcm = 0;
                return -1;
            }
        }

        aml_dec->outlen_pcm = pcm_produced_bytes;
        datmos_get_output_info(aml_dec, &(aml_dec->dec_info));
        datmos_get_atmos_info(aml_dec);

        aml_dec->is_truehd_within_mat = datmos_dec->is_truehd_within_mat;
        aml_dec->is_dolby_atmos = datmos_dec->is_dolby_atmos;

        if (aml_dec->dec_info.output_bitwidth == SAMPLE_24BITS)
            aml_dec->dec_info.output_bitwidth = SAMPLE_32BITS;


        //dump the output pcm data
        if (dump_output) {
            FILE *fp1=fopen(DATMOS_PCM_OUT_FILE,"a+");
            fwrite((char *)aml_dec->outbuf, 1, aml_dec->outlen_pcm ,fp1);
            fclose(fp1);
            aml_dec->outlen_pcm = 0;
        }


    }
    else {
        /*
         *do nothing, just store the data to aml_dec->inbuf
         *clear the output pcm length as zero.
         */
        aml_dec->outlen_pcm = 0;
    }
    ALOGV("<<ret %d OUT>>", ret);
    return 0;
EXIT:
    return -1;
}

aml_dec_func_t aml_datmos_func = {
    .f_init      = datmos_decoder_init_patch,
    .f_release   = datmos_decoder_release_patch,
    .f_process   = datmos_decoder_process_patch,
};
#endif
