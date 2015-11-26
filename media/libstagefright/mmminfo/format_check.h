/*
 * Copyright (C) 2009 Actions-Semi, Inc.
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


#ifndef __FORMAT_CHECK_H__
#define __FORMAT_CHECK_H__

#include "actal_posix_dev.h"
#include "format_dev.h"
#define LOG_TAG "format_check"
#include <utils/Log.h>
#include <dlfcn.h>
#define actal_printf ALOGD
#define actal_error ALOGE

#ifdef __cplusplus
extern "C" {
#endif

int mpccheck(storage_io_t  *input);
int mp3check(storage_io_t *input);
int aacflag(storage_io_t *input);
int wmaflag(storage_io_t *input);
int aacplusflag(storage_io_t *input);
int adts_aac_check(storage_io_t *input);
int dts_check(storage_io_t  *input);
int rm_check(storage_io_t *input);
int ts_check(storage_io_t *input);
#ifdef __cplusplus
}
#endif

#endif // __FORMAT_CHECK_H__
