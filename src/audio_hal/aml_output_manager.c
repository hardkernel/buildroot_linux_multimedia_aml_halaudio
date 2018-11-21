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


#define LOG_TAG "aml_output"
//#define LOG_NDEBUG 0

#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <unistd.h>

#include "aml_output_manager.h"
#include "alsa_manager.h"
#ifdef USE_PULSE_AUDIO
#include "pulse_manager.h"
#endif

static struct aml_output_function aml_output_function = {
	.output_open = NULL,
	.output_close = NULL,
	.output_write = NULL,
};

void aml_output_init (void) {

	ALOGD("Init the output module\n");
#ifndef USE_PULSE_AUDIO
	aml_output_function.output_open  = aml_alsa_output_open;
	aml_output_function.output_close = aml_alsa_output_close;
	aml_output_function.output_write = aml_alsa_output_write;
#else
	aml_output_function.output_open  = aml_pa_output_open;
	aml_output_function.output_close = aml_pa_output_close;
	aml_output_function.output_write = aml_pa_output_write;

#endif
	return;
}


int aml_output_open(struct audio_stream_out *stream, aml_stream_config_t * stream_config, audio_devices_t out_device) {
    int ret = -1;
    struct aml_output_handle *handle = NULL;
	output_device_t output_device = 0;
    struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;	
	ALOGD("Enter %s device=%d\n",__func__, out_device);

    if(stream == NULL || stream_config == NULL) {
		ALOGE("Input parameter is NULL\n");
		return -1;
	}

	if(aml_output_function.output_open == NULL) {
		ALOGE("Output function is NULL\n");
		return -1;
	}



    handle = (struct aml_output_handle*)calloc(1,sizeof(struct aml_output_handle));
	
	if(handle == NULL) {
		ALOGD("malloc for aml_output_handle failed\n");
        return -1;
	}
	
	memset((void*)handle, 0, sizeof(struct aml_output_handle));
	memcpy((void*)&handle->stream_config,stream_config,sizeof(aml_stream_config_t));

	handle->device = out_device;

	if(out_device == AUDIO_DEVICE_OUT_SPEAKER) {
        output_device = PCM_OUTPUT_DEVICE;
	}else if(out_device == AUDIO_DEVICE_OUT_SPDIF) {
	    output_device = RAW_OUTPUT_DEVICE;
	}
	
    ret = aml_output_function.output_open(&handle->device_handle, stream_config, out_device);
        
    if(ret == OUTPUT_ERROR) {
        free(handle);
		return -1;
	}

	aml_stream->output_handle[output_device] = handle;


    ALOGD("Exit %s = %d handle=%p\n",__func__,ret,handle);
    return ret;
}

int aml_output_close(struct audio_stream_out *stream){
    int ret = -1;
	int i = 0;
	ALOGD("Enter %s \n",__func__);
	struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
	struct aml_output_handle *output_handle = NULL;

    if(stream == NULL) {
		ALOGE("Input parameter is NULL\n");
		return -1;
	}


	if(aml_output_function.output_close == NULL) {
		ALOGE("Output function is NULL\n");
		return -1;
	}


    for (i = PCM_OUTPUT_DEVICE; i < OUTPUT_DEVICE_CNT; i ++ ) {
		output_handle = (struct aml_output_handle *)aml_stream->output_handle[i];
		// we will close all the opened output
		if(output_handle) {
			aml_output_function.output_close(output_handle->device_handle);
			output_handle->device_handle = NULL;
            free(output_handle);
            aml_stream->output_handle[i] = NULL;
		}
	}	
	 
	ALOGD("Exit %s = %d\n",__func__,ret);

    return ret;
}


int aml_output_write_pcm(struct audio_stream_out *stream, const void *buffer, int bytes){
    int ret = -1;
	struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
	struct aml_output_handle *output_handle = NULL;
	
    if(stream == NULL) {
		ALOGE("Input parameter is NULL\n");
		return -1;
	}

	if(aml_output_function.output_write == NULL) {
		ALOGE("Output function is NULL\n");
		return -1;
	}


	output_handle = (struct aml_output_handle *)aml_stream->output_handle[PCM_OUTPUT_DEVICE];

	ret = aml_output_function.output_write(output_handle->device_handle, (void*)buffer, bytes);

    return ret;
}

int aml_output_write_raw(struct audio_stream_out *stream, const void *buffer, int bytes){
    int ret = -1;
	struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
	struct aml_output_handle *output_handle = NULL;
	
    if(stream == NULL) {
		ALOGE("Input parameter is NULL\n");
		return -1;
	}

	if(aml_output_function.output_write == NULL) {
		ALOGE("Output function is NULL\n");
		return -1;
	}

	
	output_handle = (struct aml_output_handle *)aml_stream->output_handle[RAW_OUTPUT_DEVICE];

	ret = aml_output_function.output_write(output_handle->device_handle, (void*)buffer, bytes);

    return ret;
}



