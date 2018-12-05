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
#include <sys/inotify.h>

#include "log.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 16 * ( EVENT_SIZE + 16 ) )


typedef struct aml_audio_log {
    int bExit;
    pthread_t threadid;
    int notify_fd;
    int watch_desc;


} aml_audio_log_t;

static aml_audio_log_t * aml_log_handle = NULL;
int aml_audio_debug_level = LEVEL_ERROR;

void *aml_log_threadloop(void *data)
{
    aml_audio_log_t * log_handle = (aml_audio_log_t *)data;
    char *value;
    char *end;
    char *token = NULL;
    int debug_level = LEVEL_ERROR;
    char buffer[BUF_LEN];
    int len = 0;
    FILE *file_fd = NULL;
    char * debug_file = "/tmp/AML_AUDIO_DEBUG";

    prctl(PR_SET_NAME, (unsigned long)"aml_audio_log");

    while (!log_handle->bExit) {
        log_handle->watch_desc = inotify_add_watch(log_handle->notify_fd , debug_file , IN_MODIFY | IN_CREATE);
        len = read(log_handle->notify_fd, buffer, BUF_LEN);

        if (len < 0) {
            printf("read error\n");
            usleep(10 * 1000);
            continue;
        }

        file_fd = fopen(debug_file, "r");

        if (file_fd == NULL) {
            usleep(50 * 1000);
            continue;
        }

        len = fread(buffer, 1, 128, file_fd);

        if (len <= 0) {
            usleep(50 * 1000);
            continue;
        }

        if (strncmp("AML_AUDIO_DEBUG", buffer, 15)) {
            printf("wrong debug string\n");
            usleep(50 * 1000);
            continue;
        }
        token = strtok(buffer, "=");
        if (token != NULL) {
            token = strtok(NULL, "=");
            if (token != NULL) {
                debug_level = (int)strtol(token, &end, 0);
                if (debug_level != aml_audio_debug_level) {
                    printf("Debug level changed from =%d to %d\n", aml_audio_debug_level, debug_level);
                    aml_audio_debug_level = debug_level;
                }
            }
        }

        usleep(100 * 1000);
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
    aml_log_handle->notify_fd = inotify_init();
    pthread_create(&(aml_log_handle->threadid), NULL,
                   &aml_log_threadloop, aml_log_handle);

    return;
}

void aml_log_exit(void)
{
    aml_log_handle->bExit = 1;
    inotify_rm_watch( aml_log_handle->notify_fd, aml_log_handle->watch_desc);
    close(aml_log_handle->notify_fd);
    pthread_join(aml_log_handle->threadid, NULL);
    printf("Exit %s\n", __func__);
    return;
}


