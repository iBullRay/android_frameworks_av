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

#define TS_FEC_PACKET_SIZE 204
#define TS_DVHS_PACKET_SIZE 192
#define TS_PACKET_SIZE 188
#define TS_MAX_PACKET_SIZE 204

#define NULL 0

#include "format_check.h"

static int analyze(const unsigned char *buf, int size, int packet_size, int *index) {
    int stat[TS_MAX_PACKET_SIZE];
    int i;
    int x = 0;
    int best_score = 0;

    memset(stat, 0, packet_size*sizeof(int));

    for(x = i = 0; i < size-3; i++) {
        if (buf[i] == 0x47 && !(buf[i+1] & 0x80) && (buf[i+3] & 0x30)) {
            stat[x]++;
            if (stat[x] > best_score) {
                best_score = stat[x];
                if(index) *index = x;
            }
        }

        x++;
        if (x == packet_size) x = 0;
    }

    return best_score;
}

/* autodetect fec presence. Must have at least 1024 bytes  */
static int get_packet_size(const unsigned char *buf, int size) {
    int score, fec_score, dvhs_score;

    if (size < (TS_FEC_PACKET_SIZE * 5 + 1))
        return -1;

    score    = analyze(buf, size, TS_PACKET_SIZE, NULL);
    dvhs_score    = analyze(buf, size, TS_DVHS_PACKET_SIZE, NULL);
    fec_score= analyze(buf, size, TS_FEC_PACKET_SIZE, NULL);

    if (score > fec_score && score > dvhs_score) return TS_PACKET_SIZE;
    else if(dvhs_score > score && dvhs_score > fec_score) return TS_DVHS_PACKET_SIZE;
    else if(score < fec_score && dvhs_score < fec_score) return TS_FEC_PACKET_SIZE;
    else return -1;
}

int ts_check(storage_io_t *input) {
    unsigned char tsbuf[2048];
    int tssize;
    int readnum;
    
    input->seek(input, 0, SEEK_SET);
    readnum = input->read((int)tsbuf, 1, 2048, input);
    if (readnum != 2048)
        return 0;
    tssize = get_packet_size(tsbuf, 2048);
    if (tssize != -1)
        return 1;
    return 0;
}
