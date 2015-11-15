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

#ifndef __BUF_HEADER_H__
#define __BUF_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUDIO_PACKET = 0x100,
    VIDEO_PACKET = 0x200,
    SUBPIC_PACKET = 0x300
}packet_type_t;

typedef struct {
    unsigned int header_type;
    unsigned int block_len;
    unsigned int packet_offset;
    unsigned int packet_ts;
    unsigned int reserved1;
    unsigned int reserved2;
    unsigned char stream_end_flag;
    unsigned char parser_format;
    unsigned char seek_reset_flag;
    unsigned char reserved_byte2;
}packet_header_t;

#ifdef __cplusplus
}
#endif
#endif // __BUF_HEADER_H__
