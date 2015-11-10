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

#include <media/stagefright/ActAudioWriter.h>
#include <media/stagefright/MediaBuffer.h>
//#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/mediarecorder.h>
#include <sys/prctl.h>
#include <sys/resource.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "ActAudioWriter"
#include <utils/Log.h>

namespace android {
ActAudioWriter::ActAudioWriter(const char *filename) :
    mFile(fopen(filename, "wb")), mInitCheck(mFile != NULL ? OK : NO_INIT),
            mStarted(false), mPaused(false), mResumed(false) {
}

ActAudioWriter::ActAudioWriter(int fd) :
    mFile(fdopen(fd, "wb")), mInitCheck(mFile != NULL ? OK : NO_INIT),
            mStarted(false), mPaused(false), mResumed(false) {
}

ActAudioWriter::~ActAudioWriter() {
    if (mStarted) {
        ALOGD("ActAudioWriter::~stop %d", __LINE__);
        stop();
    }

    if (mFile != NULL) {
        fclose( mFile);
        mFile = NULL;
    }
}

status_t ActAudioWriter::initCheck() const {
    return mInitCheck;
}

status_t ActAudioWriter::addSource(const sp<MediaSource> &source) {
    if (mInitCheck != OK) {
        return mInitCheck;
    }

    if (mSource != NULL) {
        // files only support a single track of audio.
        return UNKNOWN_ERROR;
    }

    mSource = source;
    return OK;
}

status_t ActAudioWriter::start(MetaData *params) {
    if (mInitCheck != OK) {
        return mInitCheck;
    }

    if (mSource == NULL) {
        return UNKNOWN_ERROR;
    }

    if (mStarted && mPaused) {
        mPaused = false;
        mResumed = true;
        return OK;
    } else if (mStarted) {
        // Already started, does nothing
        return OK;
    }

    status_t err = mSource->start();
    if (err != OK) {
        return err;
    }
    MediaBuffer *buffer;
    mMeta = mSource->getFormat();
    {
        const void *data;
        uint32_t type;
        size_t n;
        mMeta->findData(kKeyIsCodecConfig, &type, &data, &n);
        if (fwrite((const uint8_t *) data, 1, n, mFile) != n) {
            ALOGE("ActAudioWriter fwrite Header err ");
            return ERROR_IO;
        }
        mMeta->setInt32(kKeyIsUnreadable, 0);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    mReachedEOS = false;
    mDone = false;

    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);

    mStarted = true;

    return OK;
}

status_t ActAudioWriter::pause() {
    if (!mStarted) {
        return OK;
    }
    ALOGV("mPauseddd  %d", mPaused);
    mPaused = true;
    return OK;
}

status_t ActAudioWriter::stop() {
    if (!mStarted) {
        return OK;
    }
    int32_t isHready = 1, cnt = 0;
    mMeta->setInt32(kKeyIsUnreadable, 1);
    ALOGE("stop: isHready 1");        
    while ((isHready != 0) && (cnt < 10)) {
        //cnt++;
        usleep(20000);  
        mMeta->findInt32(kKeyIsUnreadable, &isHready);
        ALOGE("stop: isHready %x", isHready);
    }

    mDone = true;
    void *dummy;
    pthread_join(mThread, &dummy);

    status_t err = (status_t) dummy;
    {
        status_t status = mSource->stop();
        if (err == OK && (status != OK && status != ERROR_END_OF_STREAM)) {
            err = status;
        }
        mSource.clear();
    }

    mStarted = false;
    return err;
}

bool ActAudioWriter::exceedsFileSizeLimit() {
    if (mMaxFileSizeLimitBytes == 0) {
        return false;
    }
    return mEstimatedSizeBytes >= mMaxFileSizeLimitBytes;
}

bool ActAudioWriter::exceedsFileDurationLimit() {
    if (mMaxFileDurationLimitUs == 0) {
        return false;
    }
    ALOGV("time:%ll", mEstimatedDurationUs);
    return mEstimatedDurationUs >= mMaxFileDurationLimitUs;
}

void *ActAudioWriter::ThreadWrapper(void *me) {
    return (void *) static_cast<ActAudioWriter *> (me)->threadFunc();
}

int cnt = 0;
status_t ActAudioWriter::threadFunc() {
    mEstimatedDurationUs = 0;
    mEstimatedSizeBytes = 0;
    bool stoppedPrematurely = true;
    int64_t previousPausedDurationUs = 0;
    int64_t maxTimestampUs = 0;
    status_t err = OK;
    int32_t isHready;
    prctl(PR_SET_NAME, (unsigned long) "ActAudioWriter", 0, 0, 0);
    while (!mDone) {
        MediaBuffer *buffer;
        err = mSource->read(&buffer);
        if (err != OK) {
            break;
        }

        if (mPaused) {
            ALOGV("mPaused...");
            buffer->release();
            buffer = NULL;
            usleep(50000);
            continue;
        }

        mMeta->findInt32(kKeyIsUnreadable, &isHready);
        if (isHready == 2) {
            const void *data;
            uint32_t type;
            size_t n;
            fflush( mFile);
            mMeta->findData(kKeyIsCodecConfig, &type, &data, &n);
            fseek(mFile, 0, SEEK_SET);
            if (fwrite((const uint8_t *) data, 1, n, mFile) != n) {
                ALOGE("ActAudioWriter STOP fwrite Header err ");
                return ERROR_IO;
            }
            fflush(mFile);
            ALOGD("stop: write header %x", n);
            mMeta->setInt32(kKeyIsUnreadable, 0);
            mMeta->setInt32(kKeyIsUnreadable, 0);
            break;
        }

        mEstimatedSizeBytes += buffer->range_length();
        if (exceedsFileSizeLimit()) {
            buffer->release();
            buffer = NULL;
            notify(MEDIA_RECORDER_EVENT_INFO,
                    MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED, 0);
            break;
        }

        int64_t timestampUs;
        CHECK(buffer->meta_data()->findInt64(kKeyTime, &timestampUs));
        if (timestampUs > mEstimatedDurationUs) {
            mEstimatedDurationUs = timestampUs;
        }
        if (mResumed) {
            previousPausedDurationUs += (timestampUs - maxTimestampUs - 20000);
            ALOGD("time stamp: %lld, maxTimestampUs: %lld", timestampUs,
                    maxTimestampUs);
            mResumed = false;
        }
        //timestampUs -= previousPausedDurationUs;
        //ALOGV("time stamp: %lld, previous paused duration: %lld",
        //        timestampUs, previousPausedDurationUs);
        if (timestampUs > maxTimestampUs) {
            maxTimestampUs = timestampUs;
        }

        if (exceedsFileDurationLimit()) {
            buffer->release();
            buffer = NULL;
            notify(MEDIA_RECORDER_EVENT_INFO,
                    MEDIA_RECORDER_INFO_MAX_DURATION_REACHED, 0);
            break;
        }
        /*if (cnt % 10 == 0) */{
            int timestampMs;
            timestampMs = timestampUs / 1000;
            notify(MEDIA_RECORDER_EVENT_INFO,
                    MEDIA_RECORDER_INFO_PROGRESS_TIME_STATUS, timestampMs);
        }
        cnt++;
        ssize_t n = fwrite(
                (const uint8_t *) buffer->data() + buffer->range_offset(), 1,
                buffer->range_length(), mFile);
        if (n < (ssize_t) buffer->range_length()) {
            buffer->release();
            buffer = NULL;

            break;
        }

        if (stoppedPrematurely) {
            stoppedPrematurely = false;
        }

        buffer->release();
        buffer = NULL;
    }

    if (stoppedPrematurely) {
        notify(MEDIA_RECORDER_EVENT_INFO,
                MEDIA_RECORDER_INFO_COMPLETION_STATUS, UNKNOWN_ERROR);
    }

    fflush( mFile);
    fclose(mFile);
    mFile = NULL;
    mReachedEOS = true;
    if (err == ERROR_END_OF_STREAM) {
        return OK;
    }
    return err;
}

bool ActAudioWriter::reachedEOS() {
    return mReachedEOS;
}

} // namespace android