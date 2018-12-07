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

#include <cutils/str_parms.h>
#include "aml_audio_stream.h"
#include "audio_hw.h"

int volume_control_init(struct audio_hw_device *dev);

int volume_info_setparameters(struct audio_hw_device *dev, struct str_parms *parms);

int volume_control_process(struct audio_hw_device *dev, void * in_data, size_t size, aml_data_format_t *format);


