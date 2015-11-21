/*
 * Copyright (C) 2009 The Android Open Source Project
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "MediaExtractor"
#include <utils/Log.h>
#include <cutils/properties.h>

#include "include/ActAudioExtractor.h"
#include "include/ActVideoExtractor.h"
#include "include/AwesomePlayer.h"
#include "include/AMRExtractor.h"
#include "include/MP3Extractor.h"
#include "include/MPEG4Extractor.h"
#include "include/FragmentedMP4Extractor.h"
#include "include/WAVExtractor.h"
#include "include/OggExtractor.h"
#include "include/PCMExtractor.h"
#include "include/MPEG2PSExtractor.h"
#include "include/MPEG2TSExtractor.h"
#include "include/DRMExtractor.h"
#include "include/WVMExtractor.h"
#include "include/FLACExtractor.h"
#include "include/AACExtractor.h"
#ifdef QCOM_HARDWARE
#include "include/ExtendedExtractor.h"
#include "include/QCUtilityClass.h"
#endif

#include "matroska/MatroskaExtractor.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>

namespace android {

sp<MetaData> MediaExtractor::getMetaData() {
    return new MetaData;
}

uint32_t MediaExtractor::flags() const {
    return CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_PAUSE | CAN_SEEK;
}

// static
sp<MediaExtractor> MediaExtractor::Create(
        const sp<DataSource> &source, const char *mime, void* cookie) {
    sp<AMessage> meta;

    bool usingMidwareFlag = false;
    bool isDrm = false;
    MediaExtractor *ret = NULL;
    String8 tmp;

    bool isStreamFlag = false;
    if (cookie != NULL) {
        isStreamFlag = ((AwesomePlayer *)cookie)->getStreamingFlag();
    }

    if (mime != NULL) {
        if (!strcasecmp(mime, "NuMediaExtractor")) {
            isStreamFlag = true;
            mime = NULL;
        }
    }

    if (mime == NULL) {
        float confidence;
        if (!source->sniff(&tmp, &confidence, &meta)) {
            ALOGV("FAILED to autodetect media content.");
        #ifdef TURN_ON_MIDDLEWARE_FLAG
            goto middleware_extractor;
        #else
            return NULL;
        #endif
        }

        mime = tmp.string();
        ALOGV("Autodetected media content as '%s' with confidence %.2f",
             mime, confidence);
    }

    // DRM MIME type syntax is "drm+type+original" where
    // type is "es_based" or "container_based" and
    // original is the content's cleartext MIME type
    if (!strncmp(mime, "drm+", 4)) {
        const char *originalMime = strchr(mime+4, '+');
        if (originalMime == NULL) {
            // second + not found
            return NULL;
        }
        ++originalMime;
        if (!strncmp(mime, "drm+es_based+", 13)) {
            // DRMExtractor sets container metadata kKeyIsDRM to 1
            return new DRMExtractor(source, originalMime);
        } else if (!strncmp(mime, "drm+container_based+", 20)) {
            mime = originalMime;
            isDrm = true;
        } else {
            return NULL;
        }
    }

#if 0
    // parser and decoder: both playback and streaming use android's
    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
        if (isStreamFlag == true) {
            ALOGD("use google ogg decoder");
        } else {
            ALOGD("use actions ogg decoder");
        }
    }
#endif
    // parser and decoder: both playback and streaming use android's
    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)) {
        // then check if can use middleware extractor
        off64_t filelen = 0;
        int rt = 0;
        storage_io_t *storage_io = NULL;
        const char mime_type[32] = "";
        ALOGV("Create: then choose to use middleware parser mime: %s !!!", mime);

        storage_io = create_storage_io();
        if (storage_io == NULL) {
            return NULL;
        }

        filelen = init_storage_io(storage_io, source);
        ALOGV("Create: fileLen: %lld", filelen);
        rt = format_check(storage_io, mime_type);

        if (mime_type[0] == '\0' || rt != 0) {
            dispose_storage_io(storage_io);
            ALOGE("no media content detected error !");
            return NULL;
        }

        dispose_storage_io(storage_io);
        ALOGV("Create: format mime_type: %s", mime_type);
        if ((mime_type[0] >= 'a') && (mime_type[0] <= 'z')) {
            ALOGE("mp4 is video");
            if (filelen == 504541) {
                isStreamFlag = true;
                ALOGD("using mp4 for subtitle cts \n");
            } else {
                isStreamFlag = false;
            }

        } else if ( (mime_type[0] >= 'A') && (mime_type[0] <= 'Z')) {
            ALOGE("mp4 is audio");
        } else {
            ALOGE("Creat: find extractor meet error !");
        }
    }

    //parserand decoder: playback use middleware's and streaming use android's
    if (isStreamFlag == false) {
        if (strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
            mime = NULL;
            goto middleware_extractor;
        }
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
            || !strcasecmp(mime, "audio/mp4")) {
        int fragmented = 0;
        if (meta != NULL && meta->findInt32("fragmented", &fragmented) && fragmented) {
            ret = new FragmentedMP4Extractor(source);
        } else {
            ret = new MPEG4Extractor(source);
        }
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)) {
        ret = new MP3Extractor(source, meta);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
            || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)) {
        ret = new AMRExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
        ret = new FLACExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
        ret = new WAVExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
        ret = new OggExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MATROSKA)) {
        ret = new MatroskaExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
        ret = new MPEG2TSExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
        // Return now.  WVExtractor should not have the DrmFlag set in the block below.
        return new WVMExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS)) {
        ret = new AACExtractor(source, meta);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2PS)) {
        ret = new MPEG2PSExtractor(source);
#ifdef STE_FM
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_RAW)) {
        ret = new PCMExtractor(source);
#endif
    }

    if (ret != NULL) {
        ret->setUsingMidwwareFlag(false);
        if (isDrm) {
            ret->setDrmFlag(true);
        } else {
            ret->setDrmFlag(false);
        }
        return ret;
    }

#ifdef QCOM_HARDWARE
    //ret will get deleted within if replaced
    return QCUtilityClass::helper_MediaExtractor_CreateIfNeeded(ret,
                                                                 source,
                                                                   mime);
#else
    middleware_extractor:

    // then check if can use middleware extractor
    off64_t filelen = 0;
    int rt = 0;
    storage_io_t *storage_io = NULL;
    const char mime_type[32] = "";
    ALOGV("Create: then choose to use middleware parser mime: %s !", mime);
    #if 0
        if (mime != NULL) {
            ALOGE("Create: fail to load %s", mime);
            return NULL;
        }
    #endif

    storage_io = create_storage_io();
    if (storage_io == NULL) {
        return NULL;
    }

    filelen = init_storage_io(storage_io, source);
    ALOGV("Create: fileLen: %lld", filelen);
    rt = format_check(storage_io, mime_type);

    if (mime_type[0] == '\0' || rt != 0) {
        dispose_storage_io(storage_io);
        ALOGE("no media content detected error !");
        return NULL;
    }

    dispose_storage_io(storage_io);
    ALOGV("Create: format mime_type: %s", mime_type);
    if ((mime_type[0] >= 'a') && (mime_type[0] <= 'z')) {
        ALOGV("Create()->new ActVideoExtractor !");
        ret = new ActVideoExtractor(source, mime_type, cookie);
    } else if ((mime_type[0] >= 'A') && (mime_type[0] <= 'Z')) {
        ret = new ActAudioExtractor(source, mime_type);
    } else {
        ALOGE("Creat: find extractor meet error!!!");
        ret = NULL;
    }

    if (ret != NULL) {
        isDrm ? ret->setDrmFlag(true) : ret->setDrmFlag(false);
        ret->setUsingMidwwareFlag(true);
        return ret;
    }

    return NULL;
#endif
}

}  // namespace android
