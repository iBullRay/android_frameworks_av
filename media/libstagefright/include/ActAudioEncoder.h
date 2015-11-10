/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef ACTAUDIO_ENCODER_H
#define ACTAUDIO_ENCODER_H

#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include "audio_encoder_lib_dev.h"

namespace android {

struct MediaBufferGroup;

class ActAudioEncoder: public MediaSource {
public:
    ActAudioEncoder(const sp<MediaSource> &source, const sp<MetaData> &meta);

    virtual status_t start(MetaData *params);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();
    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options);

protected:
    virtual ~ActAudioEncoder();

private:
    sp<MediaSource> mSource;
    sp<MetaData> mMeta;
    bool mStarted;
    MediaBufferGroup *mBufferGroup;
    MediaBuffer *mInputBuffer;
    status_t mInitCheck;
    int32_t mSampleRate;
    int32_t mChannels;
    int32_t mBitRate;
    int32_t mFrameCount;
    int64_t mAnchorTimeUs;
    int32_t mNumInputSamples;

    int32_t mNumSamplesPerFrame;

    audioin_pcm_t mInput;
    void * mLib_handle;
    audioenc_plugin_t *mPlugin_info;
    void *mPlugin_handle;

    status_t initCheck();

    ActAudioEncoder& operator=(const ActAudioEncoder &rhs);
    ActAudioEncoder(const ActAudioEncoder& copy);

};

}

#endif  //#ifndef ACTAUDIO_ENCODER_H