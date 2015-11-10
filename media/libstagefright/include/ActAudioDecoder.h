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

#ifndef AMP_DECODER_H_

#define AMP_DECODER_H_

#include <media/stagefright/MediaSource.h>
#include "audio_decoder_lib_dev.h"
#include "ActAudioDownMix.h"

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int32_t Int32;

typedef struct
#ifdef __cplusplus
tActAudioDecoderExternal
#endif
{
    /*
     * INPUT:
     * Pointer to the input buffer that contains the encoded bistream data.
     * The data is filled in such that the first bit transmitted is
     * the most-significant bit (MSB) of the first array element.
     * The buffer is accessed in a linear fashion for speed, and the number of
     * bytes consumed varies frame to frame.
     * The calling environment can change what is pointed to between calls to
     * the decode function, library, as long as the inputBufferCurrentLength,
     * and inputBufferUsedLength are updated too. Also, any remaining bits in
     * the old buffer must be put at the beginning of the new buffer.
     */
    uint8 *pInputBuffer;

    /*
     * INPUT:
     * Number of valid bytes in the input buffer, set by the calling
     * function. After decoding the bitstream the library checks to
     * see if it when past this value; it would be to prohibitive to
     * check after every read operation.
     */
    int32 inputBufferCurrentLength;

    /*
     * INPUT/OUTPUT:
     * Number of elements used by the library, initially set to zero by
     * the functionresetDecoder(), and modified by each
     * call to framedecoder().
     */
    int32 inputBufferUsedLength;

    /*
     * OUTPUT:
     * The number of channels decoded from the bitstream.
     */
    int16 num_channels;

    /*
     * OUTPUT:
     * The number of channels decoded from the bitstream.
     */
    int16 version;

    /*
     * OUTPUT:
     * The sampling rate decoded from the bitstream, in units of
     * samples/second.
     */
    int32 samplingRate;

    /*
     * OUTPUT:
     * This value is the bitrate in units of bits/second. IT
     * is calculated using the number of bits consumed for the current frame,
     * and then multiplying by the sampling_rate, divided by points in a frame.
     * This value can changes frame to frame.
     */
    int32 bitRate;

    /*
     * INPUT/OUTPUT:
     * In: Inform decoder how much more room is available in the output buffer in int16 samples
     * Out: Size of the output frame in 16-bit words, This value depends on the mp3 version
     */
    int32 outputFrameSize;

    /*
     * INPUT:
     * Flag to enable/disable crc error checking
     */
    int32 crcEnabled;

    /*
     * OUTPUT:
     * This value is used to accumulate bit processed and compute an estimate of the
     * bitrate. For debugging purposes only, as it will overflow for very long clips
     */
    uint32 totalNumberOfBitsUsed;

    /*
     * INPUT: (but what is pointed to is an output)
     * Pointer to the output buffer to hold the 16-bit PCM audio samples.
     * If the output is stereo, both left and right channels will be stored
     * in this one buffer.
     */

    int16 *pOutputBuffer;
    int32 total_time;
    int32_t fadeFlag;
    int32_t fadeStep;
    int32_t fadeNum;
    int32_t fadeCur;
    int32_t videoflag;
    int32_t muteNum;
} tActAudioDecoderExternal;    

namespace android {

struct MediaBufferGroup;

struct ActAudioDecoder: public MediaSource {
    ActAudioDecoder(const sp<MediaSource> &source);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options);

protected:
    virtual ~ActAudioDecoder();

private:
    sp<MediaSource> mSource;
    sp<MetaData> mMeta;

    void *mLib_handle;
    audiodec_plugin_t *mPlugin_info;
    void *mPlugin_handle;

    downmix_state* mDownMix;
    bool mStarted;

    MediaBufferGroup *mBufferGroup;
    tActAudioDecoderExternal *mConfig;
    int64_t mAnchorTimeUs;
    int64_t mNumFramesOutput;
    int64_t mNumOffsetOutput;
    
    MediaBuffer *mInputBuffer;

    void init();

    ActAudioDecoder(const ActAudioDecoder &);
    ActAudioDecoder &operator=(const ActAudioDecoder &);
};

} // namespace android

#endif  // AMP_DECODER_H_