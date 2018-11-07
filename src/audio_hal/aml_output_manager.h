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

#ifndef AML_OUTPUT_MANAGER_H
#define AML_OUTPUT_MANAGER_H

#include <tinyalsa/asoundlib.h>

#include "audio_hw.h"



enum {
    OUTPUT_OK    =  0,
    OUTPUT_ERROR = -1,
    OUTPUT_INUSE = -2
};


struct aml_output_handle {
    audio_devices_t device; /* speaker/spdif etc*/
	struct aml_stream_format stream_format;  /*stream basic info*/

	void * device_handle;


};

struct aml_output_function {
	int (*output_open)(void **handle, aml_stream_format_t * stream_format, audio_devices_t out_device);	
	void (*output_close)(void *handle);
	size_t (*output_write)(void *handle, const void *buffer, size_t bytes);

};

void aml_output_init (void);

int aml_output_open(struct audio_stream_out *stream, struct aml_stream_format * stream_format, audio_devices_t out_device);

int aml_output_close(struct audio_stream_out *stream);

int aml_output_write_pcm(struct audio_stream_out *stream, const void *buffer, int bytes);

int aml_output_write_raw(struct audio_stream_out *stream, const void *buffer, int bytes);

 
#endif
