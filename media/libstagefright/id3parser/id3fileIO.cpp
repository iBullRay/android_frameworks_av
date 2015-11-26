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

#include "id3parse.h"

namespace android {
int ID3_fread(void *buffer, size_t size, size_t count, void* stream) {   
    ID3file_t* source = (ID3file_t*)stream;
    mmm_off_t offset = (mmm_off_t)source->mOffset;
    size_t n = source->mSource->readAt64(offset, (void*)buffer, (size_t)size * count);
    if (n > 0) {
        source->mOffset += n;
    }
    return n;
    
}

int ID3_fseek(void *stream, long offset, int whence) {
    ID3file_t* source = (ID3file_t*)stream;
    if (whence == SEEK_CUR) {
        source->mOffset += offset;
    } else if (whence == SEEK_SET) {
        source->mOffset = offset;
    } else {
        source->mOffset = source->mFileSize + offset;
    }
    return 0;
}

int ID3_getfilelength(void *stream) {
    ID3file_t* source = (ID3file_t*)stream;
    return (int)source->mFileSize;
}

}
