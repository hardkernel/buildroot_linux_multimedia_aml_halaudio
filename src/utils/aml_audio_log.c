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

#include <stdlib.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <string.h>
#include <unistd.h>
#include "log.h"

typedef struct aml_audio_log {
    int bExit;
    pthread_t threadid;


} aml_audio_log_t;

static aml_audio_log_t * aml_log_handle = NULL;
int aml_audio_debug_level = LEVEL_ERROR;

void *aml_log_threadloop(void *data)
{
    aml_audio_log_t * log_handle = (aml_audio_log_t *)data;
    char *value;
    char *end;
    int debug_level = LEVEL_ERROR;

    prctl(PR_SET_NAME, (unsigned long)"aml_audio_log");

    while (!log_handle->bExit) {

        value = getenv("AML_AUDIO_DEBUG");
        if (value) {

            debug_level = (int)strtol(value, &end, 0);
            if (debug_level != aml_audio_debug_level) {
                printf("Debug level changed from =%d to %d\n", aml_audio_debug_level, debug_level);
                aml_audio_debug_level = debug_level;
            }


        }

        usleep(500 * 1000);
    }


    return ((void *) 0);
}



void aml_log_init(void)
{
    aml_log_handle = calloc(1, sizeof(aml_audio_log_t));

    if (aml_log_handle == NULL) {
        return;
    }


    aml_log_handle->bExit = 0;
    pthread_create(&(aml_log_handle->threadid), NULL,
                   &aml_log_threadloop, aml_log_handle);

    return;
}

void aml_log_exit(void)
{
    aml_log_handle->bExit = 1;
    pthread_join(aml_log_handle->threadid, NULL);
    printf("Exit %s\n", __func__);
    return;
}


