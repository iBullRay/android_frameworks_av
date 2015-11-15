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

#ifndef __VENC_COMMON_H__
#define __VENC_COMMON_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "./common/al_libc.h"
#include "./common/stream_input.h"

typedef enum {
    DISK_WRITE = 0x0,
    DISK_READ,
    DISK_RW,
}disk_flag_t;

stream_input_t* filestream_init(int cache_size);
int filestream_dispose(stream_input_t *stream_manage);
int stream_open_file(stream_input_t *stream_manage,FILE *file_handle,char *file_name,int use_size,int rw_flag);
int stream_close_file(stream_input_t *stream_manage);

#ifdef __cplusplus
}
#endif
#endif // __VENC_COMMON_H__
