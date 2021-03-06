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

#ifndef _AML_DCA_DEC_API_H_
#define _AML_DCA_DEC_API_H_

#include "audio_hal.h"
#include "aml_ringbuffer.h"
#include "aml_audio_resampler.h"
#include "aml_audio_parser.h"
#include "aml_dec_api.h"

struct dca_dts_dec {
    aml_dec_t  aml_dec;
#if 0
    unsigned char *inbuf;
    unsigned char *outbuf;
    unsigned char *outbuf_raw;
    int status;
    int remain_size;
    int outlen_pcm;
    int outlen_raw;

    int digital_raw;
    bool is_iec61937;
#endif
    int is_dtscd;
    //int (*get_parameters) (void *, int *, int *, int *);
    int (*decoder_process)(unsigned char*, int, unsigned char *, int *, char *, int *);
};

int dca_decode_init(struct aml_audio_parser *parser);
int dca_decode_release(struct aml_audio_parser *parser);
int dca_decoder_init_patch(aml_dec_t **ppdts_dec, audio_format_t format, aml_dec_config_t * dec_config);
int dca_decoder_release_patch(aml_dec_t *dts_dec);
int dca_decoder_process_patch(aml_dec_t *dts_dec, unsigned char*buffer, int bytes);

extern aml_dec_func_t aml_dca_func;

#endif
