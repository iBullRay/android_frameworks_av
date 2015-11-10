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

//#define LOG_NDEBUG 0
#define LOG_TAG "ActAudioEncoder"
#include <utils/Log.h>

#define FRAG_NUM 6
#include "include/ActAudioEncoder.h"

#include <dlfcn.h>

#include <media/stagefright/MediaBufferGroup.h>
//#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>

//#define WRITE_ENCODED_DATA
//#define WRITE_INPUT_PCM

#ifdef WRITE_ENCODED_DATA
FILE * fpOut = NULL;
#endif

#ifdef WRITE_INPUT_PCM
FILE * fpInPut = NULL;
#endif


#define LIBPREFIX "ae"
#define LIBPOSTFIX ".so"
namespace android {
ActAudioEncoder::ActAudioEncoder(const sp<MediaSource> &source,
        const sp<MetaData> &meta) :
    mSource(source), mMeta(meta), mStarted(false), mBufferGroup(NULL),
            mInputBuffer(NULL), mLib_handle(NULL), mPlugin_info(NULL) {
}

typedef int(*FuncPtr)(void);
status_t ActAudioEncoder::initCheck() {
    char* headerbuf;
    int header_len;
    const char *mime;
    char libname[64] = LIBPREFIX;
    int tmp = 0;
    FuncPtr func_handle;
    enc_audio_t enc_audio;
    audioenc_attribute_t attribute;
    CHECK(mMeta->findInt32(kKeySampleRate, &mSampleRate));
    CHECK(mMeta->findInt32(kKeyChannelCount, &mChannels));
    CHECK(mMeta->findInt32(kKeyBitRate, &mBitRate));
    mBitRate = mBitRate / 1000;
    CHECK(mMeta->findCString(kKeyMIMEType, &mime));

    if (strcmp(mime, "ADPCM") == 0) {
        strcat(libname, "WAV");
        enc_audio.audio_format = 1;
    } else {
        strcat(libname, mime);
        enc_audio.audio_format = 0;
    }
    ALOGV("AE %s s:%d ch:%d% %d;", mime, mSampleRate, mChannels, mBitRate);
    strcat(libname, LIBPOSTFIX);

    mLib_handle = dlopen(libname, RTLD_NOW);
    ALOGV("ActAudioEncoder::init-mime:%s-libname: %s; handle:%x\n", mime,libname, (int) mLib_handle);
    if (mLib_handle == NULL) {
        ALOGE("ActAudioEncoder dlopen Err: libname: %s;\n", libname);
        return UNKNOWN_ERROR;
    }
    func_handle = (FuncPtr) dlsym(mLib_handle, "get_plugin_info");
    if (func_handle == NULL) {
        ALOGE("ActAudioEncoder dlsym get_mPlugin_info Err;\n");
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return UNKNOWN_ERROR;
    }
    tmp = func_handle();
    mPlugin_info = reinterpret_cast<audioenc_plugin_t *> (tmp);
    if (mPlugin_info == NULL) {
        ALOGE("ActAudioEncoder get_mPlugin_info Err;\n");
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return UNKNOWN_ERROR;
    }
    // Configure Actions encoders
    enc_audio.sample_rate = mSampleRate;
    enc_audio.bitrate = mBitRate;
    enc_audio.channels = mChannels;
    enc_audio.chunk_time = 150;
    mPlugin_handle = mPlugin_info->open(&enc_audio);
    if (mPlugin_handle == NULL) {
        ALOGE("%s: open encoder fail\n", __FILE__);
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        mPlugin_info = NULL;
        return UNKNOWN_ERROR;
    }

    tmp = mPlugin_info->get_attribute(mPlugin_handle, &attribute);
    if (tmp < 0) {
        ALOGE("%s: get_attribute fail\n", __FILE__);
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        mPlugin_info = NULL;
        return UNKNOWN_ERROR;
    }
    ALOGV("AE_init, chunk_size = %#X, samples_per_frame = %#X\n",attribute.chunk_size, attribute.samples_per_frame);
    mNumSamplesPerFrame = attribute.samples_per_frame;
    mInput.pcm[0] = (int) malloc(sizeof(int16_t) * mNumSamplesPerFrame);
    mInput.pcm[1] = (int) malloc(sizeof(int16_t) * mNumSamplesPerFrame);
    mInput.samples = 0;
    mInput.channels = mChannels;

    memset((void *) mInput.pcm[0], 0, mNumSamplesPerFrame * 2);
    memset((void *) mInput.pcm[1], 0, mNumSamplesPerFrame * 2);

    mBufferGroup = new MediaBufferGroup;
    if (attribute.chunk_size > 0) {
        mBufferGroup->add_buffer(new MediaBuffer(attribute.chunk_size));
    } else {
        mBufferGroup->add_buffer(new MediaBuffer(20480));
    }

    mPlugin_info->update_header(mPlugin_handle, &headerbuf, &header_len);
    mMeta->setData(kKeyIsCodecConfig, 0, (const void *) headerbuf, header_len);
    mMeta->findInt32(kKeyIsUnreadable, &header_len);

#ifdef WRITE_ENCODED_DATA
    if (fpOut == NULL) {
        fpOut = fopen("/data/cz/out.mp3","wb"); 
        if (fpOut == NULL) {
            ALOGE("open output file failed");
        }
    }
#endif

#ifdef WRITE_INPUT_PCM
    if (fpInPut == NULL) {
        fpInPut = fopen("/data/cz/input.pcm","wb"); 
        if (fpInPut == NULL) {
            ALOGE("open input file failed");
        }
    }
#endif
    return OK;
}

ActAudioEncoder::~ActAudioEncoder() {
    if (mStarted) {
        ALOGV("ActAudioEncoder::~ActAudioEncoder %d", __LINE__);
        stop();
    }
    
#ifdef WRITE_ENCODED_DATA
    if (fpOut ) {
        fclose(fpOut);
        fpOut = NULL;
    }
#endif

#ifdef WRITE_INPUT_PCM
    if (fpInPut) {
        fclose(fpInPut);
        fpInPut = NULL;
    }
#endif
}

status_t ActAudioEncoder::start(MetaData *params) {
    if (mStarted) {
        ALOGD("Call start() when encoder already started");
        return OK;
    }
    CHECK_EQ((status_t)OK, initCheck());

    ALOGD("start() mNumInputSamples is zero");
    mNumInputSamples = 0;
    mAnchorTimeUs = 0;
    mFrameCount = 0;
    mSource->start(params);
    mStarted = true;
    return OK;
}

status_t ActAudioEncoder::stop() {
    if (!mStarted) {
        ALOGW("Call stop() when encoder has not started");
        return OK;
    }
    ALOGD("ActAudioEncoder stop()");

    if (mInputBuffer) {
        mInputBuffer->release();
        mInputBuffer = NULL;
    }

    delete mBufferGroup;
    mBufferGroup = NULL;
    free((void *) mInput.pcm[0]);
    free((void *) mInput.pcm[1]);

    mSource->stop();
    if (mPlugin_handle) {
        mPlugin_info->close(mPlugin_handle);
        mPlugin_handle = NULL;
    }
    if (mLib_handle) {
        dlclose( mLib_handle);
        mLib_handle = NULL;
    }
    mStarted = false;

    return OK;
}

sp<MetaData> ActAudioEncoder::getFormat() {
    sp < MetaData > srcFormat = mSource->getFormat();

    int64_t durationUs;
    if (srcFormat->findInt64(kKeyDuration, &durationUs)) {
        mMeta->setInt64(kKeyDuration, durationUs);
    }

    mMeta->setCString(kKeyDecoderComponent, "ActAudioEncoder");

    return mMeta;
}

status_t ActAudioEncoder::read(MediaBuffer **out, const ReadOptions *options) {
    status_t err;
    int32_t isCodecConfig = 0, i = 0, copy_num = 0;
    size_t copy = 0;
    uint16_t *inl = (uint16_t *) mInput.pcm[0];
    uint16_t *inr = (uint16_t *) mInput.pcm[1];
    *out = NULL;
    mMeta->findInt32(kKeyIsUnreadable, &isCodecConfig);
    if (isCodecConfig == 1) {
        char* headerbuf;
        int header_len;
        mPlugin_info->update_header(mPlugin_handle, &headerbuf, &header_len);
        mMeta->setData(kKeyIsCodecConfig, 0, (const void *) headerbuf,
                header_len);
        mMeta->setInt32(kKeyIsUnreadable, 2);
        ALOGE("ActAudioEncoder: header updata OK");
        return OK;
    }
    int64_t seekTimeUs;
    int bytes_used;
    ReadOptions::SeekMode mode;
    CHECK(options == NULL || !options->getSeekTo(&seekTimeUs, &mode));

    MediaBuffer *buffer;
    CHECK_EQ(mBufferGroup->acquire_buffer(&buffer), (status_t)OK);
    uint8_t *outPtr = (uint8_t *) buffer->data();
    bool readFromSource = false;
    int64_t wallClockTimeUs = -1;

    bytes_used = 0;
    //while(bytes_used == 0)
    {
        while (mNumInputSamples < mNumSamplesPerFrame) {
            if (mInputBuffer == NULL) {
                if (mSource->read(&mInputBuffer, options) != OK) {
                    ALOGE("read err");
                    if (mNumInputSamples == 0) {
                        buffer->release();
                        return ERROR_END_OF_STREAM;
                    }
                    memset(
                            &inl[mNumInputSamples],
                            0,
                            sizeof(int16_t) * (mNumSamplesPerFrame
                                    - mNumInputSamples));
                    memset(
                            &inr[mNumInputSamples],
                            0,
                            sizeof(int16_t) * (mNumSamplesPerFrame
                                    - mNumInputSamples));
                    mNumInputSamples = 0;
                    break;
                }

                ALOGV("audio encoder read successfully");

                size_t align = mInputBuffer->range_length() % sizeof(int16_t);
                CHECK_EQ(align, 0);

                int64_t timeUs;
                if (mInputBuffer->meta_data()->findInt64(kKeyDriftTime, &timeUs)) {
                    wallClockTimeUs = timeUs;
                }
                if (mInputBuffer->meta_data()->findInt64(kKeyAnchorTime, &timeUs)) {
                    mAnchorTimeUs = timeUs;
                }
                readFromSource = true;
            } else {
                readFromSource = false;
            }
            copy = (mNumSamplesPerFrame - mNumInputSamples) * sizeof(int16_t)
                    * mChannels;

            if (copy > mInputBuffer->range_length()) {
                copy = mInputBuffer->range_length();
            }

            copy_num = copy / (sizeof(int16_t) * mChannels);
            if (mChannels == 1) {
                //memcpy((void*) mInput.pcm[mNumInputSamples], mInputBuffer->data(),
                //      copy);
                uint16_t *tmp = (uint16_t *) (mInputBuffer->data()+mInputBuffer->range_offset());
                for (i = 0; i < copy_num; i++) {
                    inl[i + mNumInputSamples] = tmp[i] ;
                }       
            } else {
                uint16_t *tmp = (uint16_t *) (mInputBuffer->data() + mInputBuffer->range_offset());
                for (i = 0; i < copy_num; i++) {
                    inl[i + mNumInputSamples] = tmp[2 * i] ;
                    inr[i + mNumInputSamples] = tmp[2 * i + 1];
                }
            }

            mInput.samples += copy_num;

            ALOGV("audio encoder read, range_offset = %d, range_offset + copy = %d, range_length = %d, range_length - copy = %d",
                mInputBuffer->range_offset(),mInputBuffer->range_offset() + copy,mInputBuffer->range_length() ,mInputBuffer->range_length() - copy);
            mInputBuffer->set_range(mInputBuffer->range_offset() + copy,
                    mInputBuffer->range_length() - copy);

            if (mInputBuffer->range_length() == 0) {
                ALOGV("release mIput buffer");
                mInputBuffer->release();
                mInputBuffer = NULL;
            }
            mNumInputSamples += copy_num;
            if (mNumInputSamples >= mNumSamplesPerFrame) {
                mNumInputSamples %= mNumSamplesPerFrame;
                break;
            }
        }

#ifdef WRITE_INPUT_PCM
        if (fpInPut) {
            for (i = 0; i < mInput.samples ; i++) {             
                fwrite(&inl[i],1,2,fpInPut);
                fwrite(&inr[i],1,2,fpInPut);
            }   
        }
#endif
        mPlugin_info->frame_encode(mPlugin_handle, &mInput, (char *) outPtr,
                &bytes_used);
        ALOGV("audio encoder read: samples = %d, bytes_used = %d, FrameCount = %d",mInput.samples, bytes_used,mFrameCount);
        mInput.samples = 0;
        buffer->set_range(0, bytes_used);
        ++mFrameCount;
    }

#ifdef WRITE_ENCODED_DATA
    if (fpOut) {
        fwrite(outPtr,1,bytes_used,fpOut);
    }
#endif

    int64_t mediaTimeUs = ((mFrameCount - 1) * 1000000LL * mNumSamplesPerFrame)
            / mSampleRate;
    if (mFrameCount % 500 == 0) {
    }
    buffer->meta_data()->setInt64(kKeyTime, mAnchorTimeUs + mediaTimeUs);
    if (readFromSource && wallClockTimeUs != -1) {
        buffer->meta_data()->setInt64(kKeyDriftTime,
                mediaTimeUs - wallClockTimeUs);
    }

    *out = buffer;
    return OK;
}

} // namespace android