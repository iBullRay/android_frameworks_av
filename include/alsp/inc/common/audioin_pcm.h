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

#ifndef __AUDIOIN_PCM_H__
#define __AUDIOIN_PCM_H__

#define MAX_CHANNEL_IN 2

typedef struct {
    int pcm[MAX_CHANNEL_IN];
    int channels;
    int samples;
}audioin_pcm_t;

#endif // __AUDIOIN_PCM_H__
