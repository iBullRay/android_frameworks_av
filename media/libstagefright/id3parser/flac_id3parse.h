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

#ifndef _FLAC_ID3PARSE_H_
#define _FLAC_ID3PARSE_H_

#include "id3parse.h"

namespace android {
#ifdef __cplusplus
extern "C" {
#endif

enum {
    FLAC_STREAMINFO = 0,
    FLAC_PADDING = 1,
    FLAC_APPLICATION = 2,
    FLAC_SEEKTABLE = 3,
    FLAC_VORBIS_COMMENT = 4,
    FLAC_CUESHEET = 5,
    FLAC_PICTURE = 6
};

#ifdef __cplusplus
}
#endif // __cplusplus
} // namespace android

#endif
