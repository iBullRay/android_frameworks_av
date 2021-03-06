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

#ifndef __PARSER_PCM_H__
#define __PARSER_PCM_H__
#ifdef __cplusplus
extern "C" {
#endif

#define WAV_LPCM 0x1
#define WAV_MS_ADPCM 0x2
#define WAV_ALAW 0x6
#define WAV_ULAW 0x7
#define WAV_IMA_ADPCM 0x11
#define WAV_BPCM 0x21

#define WAV_ADPCM_SWF 0xd

typedef struct {    
    int32_t format;
    int32_t channels;
    int32_t samples_per_frame;
    int32_t bits_per_sample;
    int32_t bytes_per_frame;
}parser_pcm_t;

#ifdef __cplusplus
}
#endif
#endif // __PARSER_PCM_H__
