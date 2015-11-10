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

#define LOG_NDEBUG 0
#define LOG_TAG "ActAudioExtractor"

#include <utils/Log.h>
#include <dlfcn.h>
#include "include/ActAudioExtractor.h"

#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBufferGroup.h>
//#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>

#include <ActDataSource.h>

#define LIBPREFIX "ap"
#define LIBPOSTFIX ".so"
#define DEFAULT_MAX_CHUNK_SIZE (50*1024)

namespace android {

class ActAudioSource : public MediaSource {
public:
    ActAudioSource(const sp<MetaData> &meta, const sp<ActAudioExtractor> &extractor);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~ActAudioSource();

private:
    sp<ActAudioExtractor> mExtractor;
    sp<MetaData> mMeta;
    size_t mFrameSize;

    off_t mOffset;
    int64_t mCurrentTimeUs;
    bool mStarted;
    MediaBufferGroup *mGroup;

    ActAudioSource(const ActAudioSource &);
    ActAudioSource &operator = (const ActAudioSource &);
};

typedef int(*FuncPtr)(void);
ActAudioExtractor::ActAudioExtractor(const sp<DataSource>& source, const char *mime)
    : mDataSource(source), mInitCheck(NO_INIT) {
    char libname[64] = LIBPREFIX;
    int tmp = 0;
    FuncPtr func_handle;
    mPlugin_handle = NULL;

    mInput = create_storage_io();
    if (mInput == NULL) {
        ALOGE("ActAudioExtractor create_storage_io Err;\n");
        return;
    }
    init_storage_io(mInput, mDataSource);

    strcat(libname, mime);
    strcat(libname, LIBPOSTFIX);

    mLib_handle = dlopen(libname, RTLD_NOW);
    if (mLib_handle == NULL) {
        ALOGE("ActAudioExtractor dlopen Err: libname: %s;\n", libname);
        return;
    }
    func_handle = (FuncPtr) dlsym(mLib_handle, "get_plugin_info");
    if (func_handle == NULL) {
        ALOGE("ActAudioExtractor dlsym get_plugin_info Err;\n");
       if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return;
    }
    tmp = func_handle();
    mPlugin_info = reinterpret_cast<music_parser_plugin_t *>(tmp);
    if (mPlugin_info == NULL) {
        ALOGE("ActAudioExtractor get_plugin_info Err;\n");
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return;
    }

    mPlugin_handle = mPlugin_info->open(mInput);
    if (mPlugin_handle == NULL) {
        ALOGE("ActAudioExtractor: open parser fail\n");
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return;
    }
    music_info_t info;
    tmp = mPlugin_info->parser_header(mPlugin_handle, &info);
    if (tmp) {
        ALOGE("ActAudioExtractor: parser_header failed, ret = %d", tmp);
        mPlugin_info->close(mPlugin_handle);
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        mPlugin_handle = NULL;
        return;
    }

    mMeta = new MetaData;
    char mime_type[32] = "audio/";
    strcat(mime_type, info.extension);
    mMeta->setCString(kKeyMIMEType, mime_type);

    int64_t total_time = (int64_t)info.total_time * 1000000;
    if (memcmp(info.extension, PARSER_EXT_MP3, 3) == 0) {
        int resi_time = 0;
        mPlugin_info->ex_ops(mPlugin_handle, EX_OPS_GET_RESTIME, (int)&resi_time);
        total_time += (int64_t)resi_time * 1000;
    }
    // OGGһTotal time parsing, need to avoid
    if (total_time < 0) {
     ALOGE("ActAudioExtractor: total_time erro total_time = %lld",total_time);
     mPlugin_info->close(mPlugin_handle);
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        mPlugin_handle = NULL;
        return;
    }

    if (total_time == 0) {
        // For some APK time will be zero with WAV file and it request a 0.5s
        ALOGE("ActAudioExtractor: total_time zero = %lld",total_time);
        total_time = 500000;
    }
    mMeta->setInt32(kKeyChannelCount, info.channels);
    mMeta->setInt32(kKeySampleRate, info.sample_rate);
    mMeta->setInt32(kKeyBitRate, (info.avg_bitrate) * 1000);

    info.max_chunksize = (info.max_chunksize > DEFAULT_MAX_CHUNK_SIZE) ? info.max_chunksize : DEFAULT_MAX_CHUNK_SIZE;
    mMeta->setInt32(kKeyMaxInputSize, info.max_chunksize);
    mMeta->setInt64(kKeyDuration, total_time);
    mMeta->setPointer(kKeyESDS, info.buf);
    memcpy(&m_ai, &info, sizeof(music_info_t));
    strcpy(m_ai.extension, mime);
    mInitCheck = OK;
}

ActAudioExtractor::~ActAudioExtractor() {
    ALOGV("~ActAudioExtractor");
    if (mPlugin_handle != NULL) {
        mPlugin_info->close(mPlugin_handle);
        mPlugin_handle = NULL;
    }

    if (mLib_handle) {
        dlclose( mLib_handle);
        mLib_handle = NULL;
    }

    if (mInput) {
        dispose_storage_io(mInput);
        mInput = NULL;
    }
}

static void removeUnsynchronization(uint8_t *mData, int64_t mSize) {
    ALOGV("removeUnsynchronization");
    for (size_t i = 0; i + 1 < mSize; ++i) {
        if (mData[i] == 0xff && mData[i + 1] == 0x00) {
            memmove(&mData[i +1], &mData[i + 2], mSize - i - 2);
            --mSize;
        }
    }
}

typedef struct {
    char extension[8];
    char MIME[56];
} extension_type_t;

sp<MetaData> ActAudioExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;
    if (mInitCheck != OK) {
        return meta;
    }
    int i = 0;
    extension_type_t format2MIME[14] = {
        {PARSER_EXT_MP3, "audio/MP3"},
        {PARSER_EXT_AAC, "audio/AAC"},
        {PARSER_EXT_WAV, "audio/PCM"},
        {PARSER_EXT_OGG, "audio/OGG"},
        {PARSER_EXT_AMR, "audio/AMR"},
        {PARSER_EXT_ALAC, "audio/ALAC"},
        {PARSER_EXT_APE, "audio/APE"},
        {PARSER_EXT_FLAC, "audio/FLAC"},
        {PARSER_EXT_DTS, "audio/DTS"},
        {PARSER_EXT_AC3, "audio/AC3"},
        {PARSER_EXT_MPC, "audio/MPC"},
        {PARSER_EXT_AIFF, "audio/AIFF"},
        {PARSER_EXT_AA, "audio/AA"},
        {PARSER_EXT_AAX, "audio/AAX"},
    };
    {
        id3_info_total id3_info;
        int32_t type = MetaData::TYPE_NONE;
        const char *mime_type;
        void *data;
        off64_t size = 0;

        ID3file_t fp;
        fp.mSource = mDataSource;
        fp.mOffset = 0;
        mDataSource->getSize(&size);
        fp.mFileSize = size;

        mMeta->findCString(kKeyMIMEType, &mime_type);
        while (strcmp(format2MIME[i].extension, m_ai.extension) != 0) {
            i++;
            if ((i > 20) || (format2MIME[i].extension == NULL))
                return meta;
        }
        memset(&id3_info, 0, sizeof(id3_info_total));
        meta->setCString(kKeyMIMEType, format2MIME[i].MIME);
        get_audio_id3_info(&fp, m_ai.extension, &id3_info);
        if (id3_info.tag.author.content != NULL) {
            meta->setCString(kKeyArtist, (const char *)id3_info.tag.author.content);
            meta->setCString(kKeyAuthor, (const char *)id3_info.tag.author.content);
        }
        if (id3_info.tag.composer.content != NULL) {
            meta->setCString(kKeyComposer,(const char *)id3_info.tag.composer.content);
        }
        if (id3_info.tag.album.content != NULL) {
            meta->setCString(kKeyAlbum, (const char *)id3_info.tag.album.content);
        }
        if (id3_info.tag.genre.content != NULL) {
            meta->setCString(kKeyGenre, (const char *)id3_info.tag.genre.content);
        }
        if (id3_info.tag.track.content != NULL) {
            meta->setCString(kKeyCDTrackNumber,(const char *)id3_info.tag.track.content);
        }
        if (id3_info.tag.year.content != NULL) {
            meta->setCString(kKeyYear, (const char *)id3_info.tag.year.content);
        }
        if (id3_info.tag.title.content != NULL) {
            meta->setCString(kKeyTitle,(const char *)id3_info.tag.title.content);
            ALOGV("%s ",id3_info.tag.title.content);
        }
        if (id3_info.tag.comment.content != NULL) {
            meta->setCString(kKeyCompilation,(const char *)id3_info.tag.comment.content);
        }
        if (id3_info.tag.autoLoop.content != NULL) {
            const char* charData;
            charData = (const char *)id3_info.tag.autoLoop.content;
            ALOGV("auto loop %s", charData);
            if (memcmp(charData, "true", 4) == 0) {
                meta->setInt32(kKeyAutoLoop, true);
            } else {
                meta->setInt32(kKeyAutoLoop, false);
            }
        }

        if (id3_info.tag.imageInfo.length > 0) {
            char ext[32];
            strcpy(ext,"image/");
            strcat(ext,id3_info.tag.imageInfo.imageType);
            ALOGV("imageType %s len:%d offset:%d", ext,id3_info.tag.imageInfo.length,id3_info.tag.imageInfo.offset);
            data = malloc(id3_info.tag.imageInfo.length + 8096);
            if (data != NULL) {
                size_t n = mDataSource->readAt((int64_t)id3_info.tag.imageInfo.offset, data, (size_t)id3_info.tag.imageInfo.length + 8096);
                if (n > 0) {
                    if (strcmp(m_ai.extension, PARSER_EXT_MP3) == 0) {
                        uint8_t *header = (uint8_t *)malloc(10);
                        if (header) {
                            mDataSource->readAt((int64_t)0, (void*)header, 10);
                            if ((memcmp((char*)header,"ID3",3) == 0) && (header[3] == 2 || header[3] == 3)) {
                                if (header[5]&0x80) {
                                    removeUnsynchronization((uint8_t*)data,id3_info.tag.imageInfo.length + 8096);
                                }
                            }
                        }
                    }
                    meta->setData(kKeyAlbumArt, 0, (const void *)data, n);
                    int *buf = (int *)data;
                    ALOGV(" imageheader %x %x %x %x", buf[0], buf[1], buf[2], buf[3]);
                }
                free(data);
            }
            meta->setCString(kKeyAlbumArtMIME, (const char *)ext);
        }
        freeallmemory(&id3_info);
    }
    
    meta->setPointer(kKeyActMusicInfo, (void*)(&m_ai));
    return meta;
}

size_t ActAudioExtractor::countTracks() {
    return mInitCheck == OK ? 1 : 0;
}

sp<MediaSource> ActAudioExtractor::getTrack(size_t index) {
    if (mInitCheck != OK || index != 0) {
        return NULL;
    }
    return new ActAudioSource(mMeta, this);
}

sp<MetaData> ActAudioExtractor::getTrackMetaData(size_t index, uint32_t flags) {
    if (mInitCheck != OK || index != 0) {
        return NULL;
    }

    return mMeta;
}

ActAudioSource::ActAudioSource(
        const sp<MetaData> &meta, const sp<ActAudioExtractor> &extractor)
    : mExtractor(extractor),
      mMeta(meta),
      mCurrentTimeUs(0),
      mStarted(false),
      mGroup(NULL) {
}

ActAudioSource::~ActAudioSource() {
    if (mStarted == (bool)true) {
        stop();
    }
}

status_t ActAudioSource::start(MetaData *params) {
    CHECK_EQ(mStarted, false);

    int max_chunksize;
    mMeta->findInt32(kKeyMaxInputSize, &max_chunksize);
    mCurrentTimeUs = 0;
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(new MediaBuffer(max_chunksize));
    mStarted = true;

    return OK;
}

status_t ActAudioSource::stop() {
    CHECK_EQ(mStarted, true);

    delete mGroup;
    mGroup = NULL;

    mStarted = false;
    return OK;
}

sp<MetaData> ActAudioSource::getFormat() {
    return mMeta;
}

status_t ActAudioSource::read(
        MediaBuffer **out, const ReadOptions *options) {
    *out = NULL;

    int32_t n = 0, time_offset = 0;
    int64_t seekTimeUs = 0ll;
    int32_t CurTimeUs = 0ll;
    ReadOptions::SeekMode mode;
    int32_t ret = 0;
    
    MediaBuffer *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        ALOGE("acquire_buffer ERR %x",err);
        return UNKNOWN_ERROR;
    }

    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        time_offset = (int32_t)(seekTimeUs/1000);
        mExtractor->mPlugin_info->seek_time(mExtractor->mPlugin_handle, time_offset, SEEK_SET, &CurTimeUs);
        mCurrentTimeUs = (int64_t)CurTimeUs * 1000;
        buffer->meta_data()->setInt64(kKeyTime, mCurrentTimeUs);
    } else {
        buffer->meta_data()->setInt32(kKeyIsCodecConfig, 0);
        buffer->meta_data()->setInt64(kKeyTime, 0);
    }

    ret = mExtractor->mPlugin_info->get_chunk(mExtractor->mPlugin_handle, (char *)buffer->data(), &n);
    if (((ret != MP_RET_OK)&&(ret!=MP_RET_ENDFILE))||(n <= 0)) {
        buffer->release();
        buffer = NULL;
        ALOGW("read:  meet %s(return ERROR_END_OF_STREAM)  len: %d", (ret==MP_RET_OK||ret==MP_RET_ENDFILE) ? "ok" : "error", n);
        return ERROR_END_OF_STREAM;
    }

    buffer->set_range(0, n);
    mOffset += n;

    *out = buffer;

    if (ret == MP_RET_ENDFILE) {
        *out = NULL;
        buffer->release();
        buffer = NULL;
        ALOGW("ActAudioSource read: MP_RET_ENDFILE");
        return ERROR_END_OF_STREAM;
    } else {
        return OK;
    }
}

}  // namespace android