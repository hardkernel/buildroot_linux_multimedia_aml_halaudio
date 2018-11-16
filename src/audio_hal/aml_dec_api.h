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

#ifndef _AML_DEC_API_H_
#define _AML_DEC_API_H_

#include "audio_hal.h"
#include "aml_ringbuffer.h"
#include "aml_audio_resampler.h"
#include "aml_audio_parser.h"

typedef struct aml_dec_info {
    int stream_sr;    /** the sample rate in stream*/
    int stream_ch;    /** the original channels in stream*/

    int output_sr ;   /** the decoded data samplerate*/
    int output_ch ;   /** the decoded data channels*/
    int output_bitwidth; /**the decoded sample bit width*/

    int output_multi_sr ;   /** the decoded multi channel data samplerate*/
    int output_multi_ch ;   /** the decoded multi channel data channels*/
    int output_multi_bitwidth; /**the decoded multi channel sample bit width*/
    int output_bLFE;

    int bitstream_type;
    int bitstream_subtype;

} aml_dec_info_t;

typedef struct aml_dec {
    audio_format_t format;
    unsigned char *inbuf;
    unsigned char *outbuf;
    unsigned char *outbuf_raw;
    int status;
    int remain_size;
    int outlen_pcm;
    int outlen_raw;
    unsigned char *outbuf_multi;
    int outlen_multi;
    int digital_raw;
    bool is_iec61937;

    /**after decoder success, we can query these info*/
    aml_dec_info_t dec_info;
    void * dec_func;
} aml_dec_t;

typedef struct aml_dcv_config {
    int digital_raw;
    int nIsEc3;
} aml_dcv_config_t;

typedef struct aml_dca_config {
    int digital_raw;
    int is_dtscd;
} aml_dca_config_t;


typedef union aml_dec_config {
    aml_dcv_config_t dcv_config;
    aml_dca_config_t dca_config;

} aml_dec_config_t;

enum {
    AML_CONFIG_DECODER,
    AML_CONFIF_OUTPUT
};


typedef int (*F_Init)(aml_dec_t **ppaml_dec, aml_dec_config_t * dec_config);
typedef int (*F_Release)(aml_dec_t *aml_dec);
typedef int (*F_Process)(aml_dec_t *aml_dec, unsigned char* buffer, int bytes);
typedef int (*F_Config)(aml_dec_t *aml_dec, int config_type, aml_dec_config_t * config_value);


typedef struct aml_dec_func {
    F_Init    f_init;
    F_Release f_release;
    F_Process f_process;
    F_Config  f_config;
} aml_dec_func_t;



int aml_decoder_init(aml_dec_t **aml_dec, audio_format_t format, aml_dec_config_t * dec_config);
int aml_decoder_release(aml_dec_t *aml_dec);
int aml_decoder_config(aml_dec_t *aml_dec, aml_dec_config_t * config);
int aml_decoder_process(aml_dec_t *aml_dec, unsigned char*buffer, int bytes, int * used_bytes);


#endif