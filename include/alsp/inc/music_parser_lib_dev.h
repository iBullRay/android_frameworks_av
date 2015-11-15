/*
 * Copyright (C) 2008 Actions-Semi, Inc.
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

#ifndef __MUSIC_PARSER_LIB_DEV_H__
#define __MUSIC_PARSER_LIB_DEV_H__

#include "./common/al_libc.h"
#include "./common/storageio.h"
#include "./common/music_info.h"
#include "./common/extdef.h"

typedef enum {
    EX_OPS_GET_RESTIME = 0x555801,
}audioparser_ex_ops_cmd_t;

typedef enum {
    MP_RET_UNEXPECTED = -3,
    MP_RET_OUTOFMEMORY,
    MP_RET_UNSUPPORTED,
    MP_RET_OK,
    MP_RET_ENDFILE
}music_parser_ret_t;

typedef struct {
    char extension[MAX_EXT_SIZE];
    void *(*open)(storage_io_t *storage_io);
    int (*parser_header)(void *handle, music_info_t *music_info);
    int (*get_chunk)(void *handle, char *output, int *chunk_bytes);
    int (*seek_time)(void *handle, int time_offset, int whence, int *chunk_start_time);
    int (*ex_ops)(void *handle, int cmd, int args);
    void (*close)(void *handle);
}music_parser_plugin_t;

#endif // __MUSIC_PARSER_LIB_DEV_H__
