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

#include "format_check.h"

#define PAGE_SIZE 512
#define HEADER_SIZE 6

static int32_t dca_syncinfo (uint8_t * buf) {
    /* 14 bits and little endian bitstream */
    if (buf[0] == (uint8_t)0xff && buf[1] == (uint8_t)0x1f && buf[2] == (uint8_t)0x00
            && buf[3] == (uint8_t)0xe8 && (buf[4]&((uint8_t)0xf0)) == (uint8_t)0xf0
            && buf[5] == (uint8_t)0x07) {
        return 1;
    }

    /* 14 bits and big endian bitstream */
    if (buf[0] == (uint8_t)0x1f && buf[1] == (uint8_t)0xff && buf[2] == (uint8_t)0xe8
            && buf[3] == (uint8_t)0x00 && buf[4] == (uint8_t)0x07 && (buf[5]&0xf0) == (uint8_t)0xf0) {
        return 1;
    }

    /* 16 bits and little endian bitstream */
    if (buf[0] == (uint8_t)0xfe && buf[1] == (uint8_t)0x7f && buf[2] == (uint8_t)0x01
            && buf[3] == (uint8_t)0x80) {
        return 1;
    }

    /* 16 bits and big endian bitstream */
    if (buf[0] == (uint8_t)0x7f && buf[1] == (uint8_t)0xfe && buf[2] == (uint8_t)0x80
            && buf[3] == (uint8_t)0x01) {
        return 1;
    }
    return 0;
}

static INLINE uint32_t getbytes(storage_io_t *storage_io, int8_t *buffer, int32_t bytes) {
    int32_t temp;

    temp = storage_io->read(buffer, sizeof(int8_t), bytes, storage_io);
    if (temp != bytes) {
        return -1;
    }
    return temp;
}

int32_t dts_check(storage_io_t *storage_io) {
    int32_t  i= 0, ret = 0;
    uint8_t  header_buff[PAGE_SIZE+HEADER_SIZE];
    uint8_t  *buff_ptr=header_buff;

    storage_io->seek(storage_io, 0, SEEK_SET);
    ret = getbytes(storage_io,header_buff, PAGE_SIZE+HEADER_SIZE);
    if (ret < 0) {
        return -1;
    }
    for (i = 0; i < PAGE_SIZE; i++) {
        if (dca_syncinfo (buff_ptr)) {
            return 1;
        }
        buff_ptr++;
    }

    actal_memcpy(header_buff, header_buff+PAGE_SIZE, HEADER_SIZE);
    ret = getbytes(storage_io,header_buff+HEADER_SIZE, PAGE_SIZE);
    if (ret < 0) {
        return -1;
    }
    buff_ptr = header_buff;

    for (i = 0; i<PAGE_SIZE; i++) {
        if (dca_syncinfo (buff_ptr)) {
            return 1;
        }
        buff_ptr++;
    }

    return 0;
}
