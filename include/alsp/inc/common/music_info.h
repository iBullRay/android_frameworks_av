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

#ifndef __MUSIC_INFO_H__
#define __MUSIC_INFO_H__

typedef struct {
    char extension[8];
    int max_chunksize;
    int total_time;
    int avg_bitrate;
    int sample_rate;
    int channels;
    void *buf;
}music_info_t;

#endif // __MUSIC_INFO_H__
