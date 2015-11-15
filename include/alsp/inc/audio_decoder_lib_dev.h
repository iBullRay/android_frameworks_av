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

#ifndef __AUDIO_DECODER_LIB_DEV_H__
#define __AUDIO_DECODER_LIB_DEV_H__

#include "./common/audiout_pcm.h"
#include "./common/extdef.h"

typedef enum {
    EX_OPS_SPDIF_OUTPUT = 0x455801,
    EX_OPS_CHUNK_RESET = 0x455802,
}audiodec_ex_ops_cmd_t;

typedef enum {
    AD_RET_UNEXPECTED = -3,
    AD_RET_OUTOFMEMORY,
    AD_RET_UNSUPPORTED,
    AD_RET_OK,
    AD_RET_DATAUNDERFLOW,
}audiodec_ret_t;

typedef struct {
    char extension[MAX_EXT_SIZE];
    void *(*open)(void *init_buf);
    int (*frame_decode)(void *handle, const char *input, const int input_bytes, audiout_pcm_t *aout, int *bytes_used);
    int (*ex_ops)(void *handle, int cmd, int args);
    void (*close)(void *handle);
}audiodec_plugin_t;

#endif  // __AUDIO_DECODER_LIB_DEV_H__
