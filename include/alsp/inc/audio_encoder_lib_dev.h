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

#ifndef __AUDIO_ENCODER_LIB_DEV_H__
#define __AUDIO_ENCODER_LIB_DEV_H__

#include "./common/audioin_pcm.h"
#include "./common/enc_audio.h"
#include "./common/extdef.h"

typedef enum {
    AE_RET_UNEXPECTED = -7,
    AE_RET_LIBLOAD_ERROR,
    AE_RET_LIB_ERROR,
    AE_RET_ENCODER_ERROR,
    AE_RET_FS_ERROR,
    AE_RET_OUTOFMEMORY,
    AE_RET_UNSUPPORTED,
    AE_RET_OK,
    AE_RET_DATAOVERFLOW,
}audioenc_ret_t;

typedef struct {
    int samples_per_frame;
    int chunk_size;
    void *buf;
    int buf_len;
}audioenc_attribute_t;

typedef struct {
    char extension[MAX_EXT_SIZE];
    void *(*open)(enc_audio_t *enc_audio);
    int (*get_attribute)(void *handle, audioenc_attribute_t *attribute);
    int (*update_header)(void *handle, char **header_buf, int *header_len);
    int (*frame_encode)(void *handle, audioin_pcm_t *ain, char *output, int *bytes_used);
    void (*close)(void *handle);
}audioenc_plugin_t;

#endif // __AUDIO_ENCODER_LIB_DEV_H__
