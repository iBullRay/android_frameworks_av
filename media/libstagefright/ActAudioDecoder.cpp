/*
 * Copyright (C) 2009 The Android Open Source Project
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
#define LOG_TAG "ActAudioDecoder"
#include <utils/Log.h>

#include "include/ActAudioDecoder.h"
#include "include/ActAudioExtractor.h"

#include <dlfcn.h>

#include <media/stagefright/MediaBufferGroup.h>
//#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#define LIBPREFIX "ad"
#define LIBPOSTFIX ".so"

//#define DEBUG

#define PP_MAXVALUE 0X7FFF
#define DEFAULT_SAMPLES 8192
#define DEFAULT_CHANNELS 2
#define DEFAULT_FADETIME 600
#define DEFAULT_MUTETIME 200
#define MIN_FADETIME 50

namespace android {

ActAudioDecoder::ActAudioDecoder(const sp<MediaSource> &source) :
    mSource(source), mLib_handle(NULL), mPlugin_handle(NULL), mDownMix(NULL),
            mStarted(false), mBufferGroup(NULL),
            mConfig(new tActAudioDecoderExternal), mAnchorTimeUs(0),
            mNumFramesOutput(0), mNumOffsetOutput(0), mInputBuffer(NULL) {
    init();
}

typedef int(*FuncPtr)(void);

void ActAudioDecoder::init() {
    sp < MetaData > srcFormat = mSource->getFormat();
    const char *mime;
    int32_t tmp = 0;
    char libname[64] = LIBPREFIX;
    FuncPtr func_handle;
    memset(mConfig, 0, sizeof(tActAudioDecoderExternal));
    CHECK(srcFormat->findInt32(kKeyChannelCount, &tmp));
    CHECK(srcFormat->findInt32(kKeySampleRate, &mConfig->samplingRate));
    CHECK(srcFormat->findCString(kKeyMIMEType, &mime));
    if (srcFormat->findInt64(kKeyDriftTime, &mNumOffsetOutput)) {
        mAnchorTimeUs = ((uint64_t) mNumOffsetOutput >> 32) * 1000;
        mNumOffsetOutput = (uint64_t) mNumOffsetOutput & (uint64_t) 0xffffffff;
        if (mNumOffsetOutput > 0) {
            mNumOffsetOutput = mNumOffsetOutput
                    * (mConfig->samplingRate / 1000);
        }
    }

    if (tmp > 2) {
        ALOGE("mNumChannels %d", tmp);
        tmp = 2;
    }
    mConfig->num_channels = (int16) tmp;
    mMeta = new MetaData;
    mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
    mMeta->setInt32(kKeyChannelCount, tmp);
    mMeta->setInt32(kKeySampleRate, mConfig->samplingRate);

    int64_t durationUs;
    if (srcFormat->findInt64(kKeyDuration, &durationUs)) {
        mMeta->setInt64(kKeyDuration, durationUs);
        mConfig->total_time = durationUs / 1000000;
    }
    srcFormat->findInt64(kKeyDuration, &durationUs);
    mMeta->setCString(kKeyDecoderComponent, "ActAudioDecoder");

    strcat(libname, mime + sizeof("audio"));
    strcat(libname, LIBPOSTFIX);

    mLib_handle = dlopen(libname, RTLD_NOW);
    ALOGD("ActAudioDecoder::init-mime:%s-libname: %s; handle:%x\n", mime,
            libname, (int) mLib_handle);
    if (mLib_handle == NULL) {
        ALOGE("ActAudioDecoder::init-mime:%s-libname: %s; handle:%x\n", mime,
                libname, (int) mLib_handle);
        return;
    }

    func_handle = (FuncPtr) dlsym(mLib_handle, "get_plugin_info");
    if (func_handle == NULL) {
        ALOGE(" dlsym get_mPlugin_info Err;\n");
        dlclose( mLib_handle);
        mLib_handle = NULL;
        return;
    }
    tmp = func_handle();
    mPlugin_info = reinterpret_cast<audiodec_plugin_t *> (tmp);
    if (mPlugin_info == NULL) {
        dlclose( mLib_handle);
        mLib_handle = NULL;
        return;
    }
    mDownMix = downmix_open();
    if (mDownMix == NULL) {
        dlclose( mLib_handle);
        mLib_handle = NULL;
        return;
    }
}

ActAudioDecoder::~ActAudioDecoder() {
    if (mStarted) {
        stop();
    }
    if (mDownMix != NULL) {
        downmix_close( mDownMix);
        mDownMix = NULL;
    }

    if (mPlugin_handle) {
        mPlugin_info->close(mPlugin_handle);
        mPlugin_handle = NULL;
        dlclose( mLib_handle);
        mLib_handle = NULL;
    }

    delete mConfig;
    mConfig = NULL;
    ALOGD("~ActAudioDecoder");
}
#ifdef DEBUG
FILE *dec_fp = NULL;
#endif

status_t ActAudioDecoder::start(MetaData *params) {
    CHECK(!mStarted);
    if (mLib_handle == NULL) {
        ALOGD("Err: Audio mLib_handle is NULL\n");
        return ERROR_UNSUPPORTED;
    }
    ALOGV("ActAudioDecoder::start");
    if (mConfig->samplingRate >= 96000) {
        return ERROR_UNSUPPORTED;
    }
    sp < MetaData > srcFormat = mSource->getFormat();
    void *init_buf = NULL;
    srcFormat->findPointer(kKeyESDS, &init_buf);
    mPlugin_handle = mPlugin_info->open(init_buf);
    //void *info;
    //srcFormat->findPointer(kKeyESDS, &info);
    //mPlugin_handle = mPlugin_info->open(((music_info_t *)info)->buf);
    
    if (mPlugin_handle == NULL) {
        ALOGD("Err: Audio mPlugin_handle is NULL\n");
        return ERROR_UNSUPPORTED;
    }

#ifdef DEBUG
    if (dec_fp != NULL) {
        fclose( dec_fp);
        dec_fp = NULL;
    }
    dec_fp = fopen("/data/tmp.pcm", "wb");
    if (dec_fp == NULL) {
        ALOGE("creat tmp file failed!");
    }
#endif
    if (mConfig->total_time > 10) {
        mConfig->fadeFlag = 1;
        mConfig->fadeCur = 0;
        mConfig->fadeNum = mConfig->samplingRate / 1000 * DEFAULT_FADETIME;
        mConfig->muteNum = mConfig->samplingRate / 1000 * DEFAULT_MUTETIME;
        mConfig->fadeStep = PP_MAXVALUE / mConfig->fadeNum;
        if (mConfig->fadeStep < 1) {
            mConfig->fadeStep = 1;
            mConfig->fadeNum = mConfig->samplingRate;
        }
    }
    mSource->start();

    //mAnchorTimeUs = 0;
    mNumFramesOutput = 0;
    mStarted = true;

    return OK;
}

status_t ActAudioDecoder::stop() {
    ALOGE("ActAudioDecoder::stop");
    CHECK( mStarted);

    ALOGV("ActAudioDecoder::stop");
    if (mInputBuffer) {
        mInputBuffer->release();
        mInputBuffer = NULL;
    }
#ifdef DEBUG
    if (dec_fp != NULL) {
        fclose( dec_fp);
        dec_fp = NULL;
    }
#endif
    //free(mPlugin_handle);
    delete mBufferGroup;
    mBufferGroup = NULL;

    mSource->stop();

    mStarted = false;

    return OK;
}

sp<MetaData> ActAudioDecoder::getFormat() {
    return mMeta;
}
static int post_sat(int acc, int post_max, int post_min) {
    if (acc > post_max) {
        acc = post_max;
    } else if (acc < post_min) {
        acc = post_min;
    }
    return acc;
}

static int mix_data(audiout_pcm_t *out, int16 *output,
        downmix_state *dommix_str) {
    int i = 0;
    int16 *tmp_buf = output;
    int16 fadecoef, fade_step = 0;
    int *pl = (int *) out->pcm[0];
    int *pr = (int *) out->pcm[1];
    int sat_min = (int) (~0) << 15;
    int sat_max = ~sat_min;

    if (out->channels > 2) {
        downmix_set(dommix_str, (downmix_int32 *) (out->pcm[0]),
                (downmix_int32 *) (out->pcm[1]));
        for (i = 0; i < out->channels; i++) {
            downmix_run(dommix_str, (downmix_int32 *) (out->pcm[i]),
                    out->samples, i);
        }
    }

    if (out->channels == 1) {
        for (i = 0; i < out->samples; i++) {
            *output++ = (short) post_sat((*pl++ >> out->frac_bits), sat_max,
                    sat_min);
            //*output++ =  (short)(*pl++ >> out->frac_bits);
        }
    } else {
        for (i = 0; i < out->samples; i++) {
            *output++ = (short) (post_sat((*pl++ >> out->frac_bits), sat_max,
                    sat_min));
            *output++ = (short) (post_sat((*pr++ >> out->frac_bits), sat_max,
                    sat_min));
        }
    }
    return 0;
}

inline int16 mult16(int16 a, int16 b) {
    int i = a, j = b, k = 0;
    short m;
    k = i * j;
    m = (short) (k >> 16);
    return m;
}

static void fadeproc(tActAudioDecoderExternal *cfg, int32_t len) {
    int16 *tmp_buf, *output;
    tmp_buf = output = cfg->pOutputBuffer;
    int i, fadecoef;
    if (cfg->fadeFlag == 1) {
        fadecoef = cfg->fadeCur * cfg->fadeStep;
    } else {
        fadecoef = PP_MAXVALUE + cfg->fadeCur * cfg->fadeStep;
    }
    ALOGV("fade %d %d %d %d %d", cfg->fadeFlag, cfg->fadeStep, cfg->fadeNum,
            cfg->fadeCur, fadecoef);
    if (cfg->num_channels == 1) {
        for (i = 0; i < len; i++) {
            if (cfg->muteNum > 0) {
                *tmp_buf++ = 0;
                cfg->muteNum--;
            } else {
                *tmp_buf++ = mult16(*output++, fadecoef);
                fadecoef += cfg->fadeStep;
                cfg->fadeCur++;
                if (cfg->fadeCur >= cfg->fadeNum) {
                    if (cfg->fadeFlag == 1) {
                        cfg->fadeFlag = 0;
                        return;
                    } else {
                        *tmp_buf++ = 0;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < len; i++) {
            if (cfg->muteNum > 0) {
                *tmp_buf++ = 0;
                *tmp_buf++ = 0;
                cfg->muteNum--;
            } else {
                *tmp_buf++ = mult16(*output++, fadecoef);
                *tmp_buf++ = mult16(*output++, fadecoef);
                fadecoef += cfg->fadeStep;
                cfg->fadeCur++;
                if (cfg->fadeCur >= cfg->fadeNum) {
                    if (cfg->fadeFlag == 1) {
                        cfg->fadeFlag = 0;
                        return;
                    } else {
                        *tmp_buf++ = 0;
                        *tmp_buf++ = 0;
                    }
                }
            }
        }
    }
}

status_t ActAudioDecoder::read(MediaBuffer **out, const ReadOptions *options) {
    status_t err;
    int64_t seekTimeUs, timestamp;
    ReadOptions::SeekMode mode;
    MediaBuffer *buffer;
    audiout_pcm_t pcmout;
    int bytes_used = 0;
    int ret = 0;
    *out = NULL;

    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        CHECK(seekTimeUs >= 0);
        mNumFramesOutput = 0;
        
        ALOGE("seekTimeUs=%lld",seekTimeUs);
        
        if ((mConfig->total_time > 5) && (mConfig->videoflag == 2)) {
            mConfig->fadeFlag = 1; //set fadein
            mConfig->fadeCur = 0;
            mConfig->fadeNum = mConfig->samplingRate / 1000 * MIN_FADETIME;
            mConfig->muteNum = mConfig->samplingRate / 1000 * DEFAULT_MUTETIME;
            mConfig->fadeStep = PP_MAXVALUE / mConfig->fadeNum;
        }
        if (mInputBuffer) {
            mInputBuffer->release();
            mInputBuffer = NULL;
        }
        // Make sure that the next buffer output does not still
        // depend on fragments from the last one decoded.
        mPlugin_info->ex_ops(mPlugin_handle, EX_OPS_CHUNK_RESET, 0);
        mNumOffsetOutput = 0;
    } else {
        seekTimeUs = -1;
    }

    if (mInputBuffer == NULL) {
        err = mSource->read(&mInputBuffer, options);
        if (err != OK) {
            if (err == INFO_DISCONTINUITY) {
                ALOGD("audio received INFO_DISCONTINUITY");
                if (mBufferGroup == NULL) {
                    mBufferGroup = new MediaBufferGroup;
                    mBufferGroup->add_buffer(
                            new MediaBuffer(
                                    DEFAULT_CHANNELS * DEFAULT_SAMPLES
                                            * sizeof(int16_t)));
                }

                CHECK_EQ(mBufferGroup->acquire_buffer(&buffer), (status_t)OK);

                buffer->set_range(0, 0);

                mAnchorTimeUs = 0;
                mNumFramesOutput = 0;
                mPlugin_info->ex_ops(mPlugin_handle, EX_OPS_CHUNK_RESET, 0);
                mNumOffsetOutput = 0;
                buffer->meta_data()->setInt64(kKeyTime, 0);

                *out = buffer;

                return OK;
            }
            ALOGE("mSource->read(&mInputBuffer, options) err!!!");
            return err;
        } else {
            mInputBuffer->meta_data()->findInt32(kKeyIsCodecConfig, &mConfig->videoflag);
            if (mConfig->videoflag > 0) {
                ALOGV("seek kKeyIsCodecConfig %d", mConfig->videoflag);;
                mPlugin_info->ex_ops(mPlugin_handle, EX_OPS_CHUNK_RESET, 0);
                if (mConfig->total_time > 5) {
                    mConfig->fadeFlag = 1; //set fadein
                    mConfig->fadeCur = 0;
                    mConfig->fadeNum = mConfig->samplingRate / 1000
                            * MIN_FADETIME;
                    mConfig->fadeStep = PP_MAXVALUE / mConfig->fadeNum;
                    mNumOffsetOutput = 0;
                }
            }
        }

        if (options && options->getSeekTo(&seekTimeUs, &mode)) {
            int64_t timeUs;
            if (mInputBuffer->meta_data()->findInt64(kKeyTime, &timeUs)) {
                if (((seekTimeUs - timeUs) > 2000000) || ((seekTimeUs - timeUs)
                        < -2000000)) {
                    ALOGW("audiodec-seek time: %lld %lld", timeUs, seekTimeUs);
                    timeUs = seekTimeUs;
                }
                mAnchorTimeUs = timeUs;
                mNumFramesOutput = 0;
            } else {
                // We must have a new timestamp after seeking.
                CHECK(seekTimeUs < 0);
            }
        }
    }
    mConfig->pInputBuffer = (uint8_t *) mInputBuffer->data()
            + mInputBuffer->range_offset();
    mConfig->inputBufferCurrentLength = mInputBuffer->range_length();

    mConfig->inputBufferUsedLength = 0;
    mConfig->outputFrameSize = 0;
    if (mConfig->inputBufferCurrentLength <= 0) {
        ALOGW("AD input is null");
        return OK;
    }

    ret = mPlugin_info->frame_decode(mPlugin_handle,
            (char *) mConfig->pInputBuffer, mConfig->inputBufferCurrentLength,
            &pcmout, &bytes_used);
    if (ret < 0) {
        ALOGE("decoder returned error %d", ret);
        // This is recoverable, just ignore the current frame and
        // play silence instead.
        //if (mBufferGroup != NULL) {
        //  CHECK_EQ(mBufferGroup->acquire_buffer(&buffer), (status_t)OK);
        //  mConfig->pOutputBuffer = static_cast<int16_t *>(buffer->data());
        //  memset(buffer->data(), 0, buffer->size());
        //  mConfig->inputBufferUsedLength = mInputBuffer->range_length();
        //}
        return UNKNOWN_ERROR;
    }

    if (mBufferGroup == NULL) {
        mBufferGroup = new MediaBufferGroup;
        mBufferGroup->add_buffer(
                new MediaBuffer(
                        DEFAULT_CHANNELS * DEFAULT_SAMPLES * sizeof(int16_t)));
    }

    CHECK_EQ(mBufferGroup->acquire_buffer(&buffer), (status_t)OK);
    int tmp1 = buffer->size();
    int tmp2 = DEFAULT_CHANNELS * pcmout.samples * sizeof(int16_t);
    if (tmp1 < tmp2) {
        ALOGW("buf size is change o:%x n:%d %d", tmp1, tmp2, pcmout.channels);
        buffer->release();
        delete mBufferGroup;
        mBufferGroup = new MediaBufferGroup;
        mBufferGroup->add_buffer(new MediaBuffer(tmp2));
        CHECK_EQ(mBufferGroup->acquire_buffer(&buffer), (status_t)OK);
    }
    mConfig->pOutputBuffer = static_cast<int16_t *> (buffer->data());

    mNumOffsetOutput -= pcmout.samples;
    if ((mNumOffsetOutput > 0) || (pcmout.channels < 1) || (pcmout.samples < 1)) {
        ALOGV("mix (ch:%d,samples:%d)", pcmout.channels, pcmout.samples);
        pcmout.channels = 0;
        pcmout.samples = 0;
        goto out;
    }

    if ((pcmout.channels > mConfig->num_channels)
            && (mConfig->num_channels < 2)) {
        pcmout.channels = mConfig->num_channels;
    }

    ret = mix_data(&pcmout, mConfig->pOutputBuffer, mDownMix);

    if (mConfig->fadeFlag != 0) {
        fadeproc(mConfig, pcmout.samples);
    }

    out: mConfig->outputFrameSize = pcmout.samples * mConfig->num_channels; // default: 2 channels; 16bit

    if (bytes_used > mConfig->inputBufferCurrentLength) {
        ALOGW("decoder maybe overflow u%d  i%d", bytes_used,
                mConfig->inputBufferCurrentLength);
        bytes_used = mConfig->inputBufferCurrentLength;
    }
    mConfig->inputBufferUsedLength = bytes_used;
    mConfig->totalNumberOfBitsUsed += bytes_used << 3; //bit units
    buffer->set_range(0, mConfig->outputFrameSize * sizeof(int16_t));
    mInputBuffer->set_range(
            mInputBuffer->range_offset() + mConfig->inputBufferUsedLength,
            mInputBuffer->range_length() - mConfig->inputBufferUsedLength);

    if (mInputBuffer->range_length() == 0) {
        mInputBuffer->release();
        mInputBuffer = NULL;
    }
    timestamp = mAnchorTimeUs + (mNumFramesOutput * 1000000)
            / mConfig->samplingRate;
    int32_t tmptime = (int32_t)((timestamp + 1500000) / 1000000);
//  if ((tmptime > 10) && (tmptime > mConfig->total_time) &&(mConfig->fadeFlag == 0)) {
//      mConfig->fadeFlag = 2;
//      mConfig->fadeCur = 0;
//      mConfig->fadeNum = mConfig->samplingRate / 1000 * DEFAULT_FADETIME;
//      mConfig->fadeStep = -PP_MAXVALUE / mConfig->fadeNum;
//      if (mConfig->fadeStep == 0) {
//          mConfig->fadeStep = -1;
//          mConfig->fadeNum = mConfig->samplingRate;
//      }
//  }

    buffer->meta_data()->setInt64(kKeyTime, timestamp);

    mNumFramesOutput += mConfig->outputFrameSize / mConfig->num_channels;
#ifdef DEBUG
    if (dec_fp != NULL)
    fwrite(mConfig->pOutputBuffer, 2, mConfig->outputFrameSize, dec_fp);
#endif
    *out = buffer;

    return OK;
}

} // namespace android