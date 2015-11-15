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

#ifndef __AUDIOUT_PCM_H__
#define __AUDIOUT_PCM_H__

#define MAX_CHANNEL 6

typedef struct {
    int pcm[MAX_CHANNEL];
    int channels;
    int samples;
    int frac_bits;
}audiout_pcm_t;

#endif // __AUDIOUT_PCM_H__
