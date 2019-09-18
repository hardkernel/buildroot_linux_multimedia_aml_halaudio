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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <unistd.h>
#include "log.h"
#include <sys/prctl.h>
#include "properties.h"
#include <dlfcn.h>

#include "dolby_lib_api.h"

#ifndef RET_OK
#define RET_OK 0
#endif

#ifndef RET_FAIL
#define RET_FAIL -1
#endif


#define DOLBY_MS12_LIB_PATH_A "/vendor/lib/libdolbyms12.so"
#define DOLBY_MS12_LIB_PATH_B "/system/vendor/lib/libdolbyms12.so"

//#define DOLBY_DCV_LIB_PATH_A "/vendor/lib/libHwAudio_dcvdec.so"
#define DOLBY_DCV_LIB_PATH_A "/usr/lib/libdcv.so"
#define DOLBY_ATMOS_LIB_PATH_A "/usr/lib/libdolby_atmos.so"


/*
 *@brief file_accessible
 */
static int file_accessible(char *path)
{
    // file is readable or not
    if (access(path, R_OK) == 0) {
        return RET_OK;
    } else {
        ALOGE("path %s can not access!\n", path);
        return RET_FAIL;
    }
}


/*
 *@brief detect_dolby_lib_type
 */
enum eDolbyLibType detect_dolby_lib_type(void) {
    enum eDolbyLibType retVal;

    // the priority would be "MS12 > Atmos >DCV" lib
    if (RET_OK == file_accessible(DOLBY_MS12_LIB_PATH_A))
    {
        retVal = eDolbyMS12Lib;
    } else if (RET_OK == file_accessible(DOLBY_MS12_LIB_PATH_B))
    {
        retVal = eDolbyMS12Lib;
    } else if (RET_OK == file_accessible(DOLBY_ATMOS_LIB_PATH_A))
    {
        retVal = eDolbyAtmosLib;
    } else if (RET_OK == file_accessible(DOLBY_DCV_LIB_PATH_A))
    {
        retVal = eDolbyDcvLib;
    } else {
        retVal = eDolbyNull;
    }

    return retVal;
}


