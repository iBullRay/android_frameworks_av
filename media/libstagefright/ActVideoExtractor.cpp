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

#define LOG_NDEBUG 0
#define LOG_TAG "ActVideoExtractor"
#include <utils/Log.h>
#include <dlfcn.h>

#include <binder/MemoryDealer.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <utils/String8.h>

/* for notify */
#include "include/AwesomePlayer.h"

#include "include/ActVideoExtractor.h"
#include "ActDataSource.h"

#define WORD_SIZE 0x04
#define WORD_MASK 0xfffffffc

#define ONLY_PLAY_VIDEO_FLAG 0
#define ONLY_PLAY_AUDIO_FLAG 0
#define OUTPUT_PARSER_DATA_A 0


#define EXTRACT_LOGV(x, ...)
#define EXTRACT_LOGD(x, ...)
#define BUFFER_LOGD(x, ...)
#define EXTRACT_LOGW(x, ...)
#define EXTRACT_LOGT(x, ...)
#define SUB_LOGD(x, ...)
#define AUDIOTRACK_LOGD(x, ...)

namespace android {
#define UNKNOWN_PACKET (SUBPIC_PACKET+1)
#define SIZE_PHT (sizeof(packet_header_t))

#define MINIMUM_LASTPK_TIMEDIFF 100000 // 100 MS
#define DIFFTOEND 5000000
#define MAX_GET_PACKETS_LOOP_TIMES 700

#define USE_PHY_CONTINUAL_STREAM_BUF

#ifdef USE_PHY_CONTINUAL_STREAM_BUF
#include <common/al_libc.h>
#endif

static const ExtToMime_t kExtToMime[] = {
    {"", "video/unsupport",  CODEC_UNKNOWN},
    {"h264", MEDIA_MIMETYPE_VIDEO_AVC, CODEC_ACTIONS},
    {"h263", MEDIA_MIMETYPE_VIDEO_H263, CODEC_ACTIONS},
    {"divx", MEDIA_MIMETYPE_VIDEO_MPEG4, CODEC_ACTIONS},
    {"div3", MEDIA_MIMETYPE_VIDEO_DIV3, CODEC_ACTIONS},
    {"msm4", MEDIA_MIMETYPE_VIDEO_DIV3, CODEC_ACTIONS},
    {"mpeg", MEDIA_MIMETYPE_VIDEO_MPEG2, CODEC_ACTIONS},
    {"flv", MEDIA_MIMETYPE_VIDEO_FLV, CODEC_ACTIONS},
    {"wmv8" , MEDIA_MIMETYPE_VIDEO_WMV8, CODEC_ACTIONS},
    {"misc", MEDIA_MIMETYPE_VIDEO_WMV8, CODEC_ACTIONS},
    {"vp3t", MEDIA_MIMETYPE_VIDEO_WMV8, CODEC_ACTIONS},
    {"", "video/unsupport", CODEC_UNKNOWN}
};

static const aExtToMime_t kAExtToMime[] = {
    {"" , "unsupport", 9},
    {"aac", "AAC", 3},
    {"aac-adts", "AAC", 3},
    {"ac3", "AC3", 3},
    {"aiff", "AIFF",  4},
    {"amr", "AMR", 3},
    {"ape", "APE", 3},
    {"dts", "DTS",  3},
    {"flac", "FLAC", 4},
    {"mp3", "MP3", 3},
    {"mpc", "MPC", 3},
    {"ogg", "OGG", 3}, 
    {"pcm", "PCM", 3}, 
    {"wav", "PCM", 3},
    {"", "unsupport", 9}
};

struct ActVideoSource : public MediaSource {
    ActVideoSource(
        const sp<ActVideoExtractor> &extractor, size_t index);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
        MediaBuffer **buffer, const ReadOptions *options);

    protected:
    virtual ~ActVideoSource();

    private:
        enum Type {
            AVC,
            AAC,
            OTHER
        };

    sp<ActVideoExtractor> mExtractor;
    mutable Mutex mLock;
    Mutex mMiscStateLock;
    size_t mTrackIndex;
    size_t mReadAgainCouter;
    Type mType;
    packet_header_t *m_pht;
#if OUTPUT_PARSER_DATA_A
    FILE * fp_audio_data;
#endif
    ActVideoSource(const ActVideoSource &);
    ActVideoSource &operator=(const ActVideoSource &);
};

ActVideoSource::ActVideoSource(
        const sp<ActVideoExtractor> &extractor, size_t index)
    : mExtractor(extractor),
      mTrackIndex(index),
      mType(OTHER) {
    //sp<MetaData> meta = mExtractor->mTracks.itemAt(index).mMeta;
    mExtractor->mVideoAlreadySeek = 0;
    mReadAgainCouter = 0;
#ifdef MMM_ENABLE_SUBTITLE  
    actal_memset(mExtractor->subtitle_data,0,MMM_SUBTITLE_DATA_LEN*MMM_SUBTITLE_NUM);
    mExtractor->subtitle_buf_No_0 = 0;
    mExtractor->subtitle_buf_No_1 = 0;
    mExtractor->subtitle_get_flag = 0;
    mExtractor->pkt_prv_ts = -1; 
    mExtractor->packet_header_tmp = NULL;
    mExtractor->packet_header_org = NULL;
    mExtractor->addr_oft = 0;   
    mExtractor->reserved_count = 0;
    mExtractor->put_subtitle_packet_flag = 0;
    mExtractor->need_subtitle = 0;
#endif     
#if OUTPUT_PARSER_DATA_A
    fp_audio_data = fopen("/data/juan/parser_out.data", "a+");
    if (fp_audio_data == NULL) {
        ALOGE("ActVideoSource:open /data/juan/parser_out.data error");
    }
#endif
}

ActVideoSource::~ActVideoSource() {
#if OUTPUT_PARSER_DATA_A
    if (fp_audio_data) {
        fclose(fp_audio_data);
    }
#endif
    EXTRACT_LOGV("~ActVideoSource");
}

status_t ActVideoSource::start(MetaData *params) {
    return OK;
}

status_t ActVideoSource::stop() {
    return OK;
}

sp<MetaData> ActVideoSource::getFormat() {
    return mExtractor->mTracks.itemAt(mTrackIndex).mMeta;
}

status_t ActVideoSource::read(MediaBuffer **out, const ReadOptions *options) {
    Mutex::Autolock autoLock(mLock);

    EXTRACT_LOGV("Entry read: type is %s", mTrackIndex == 0 ? "video" : "audio");
    packet_type_t t = mExtractor->matchPacketType(mTrackIndex);
    if (t == (packet_type_t)UNKNOWN_PACKET) {
        return ERROR_OUT_OF_RANGE;
    }

    int64_t seekTimeUs = 0;
    ReadOptions::SeekMode mode;
    bool seeked = false;
    status_t ret = OK;
    bool isAudioFlag = (t == VIDEO_PACKET) ? false : true;
    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        EXTRACT_LOGT("read: seek to %lld isAudioFlag: %s", seekTimeUs, isAudioFlag==true ? "true" : "false");
        seeked = true;
        if (true == mExtractor->seekable()) {
            if (isAudioFlag == false || (mExtractor->mNotPlayVideo || mExtractor->mNotPlayAudio)) {
                EXTRACT_LOGD("read: isAudioFlag %d, mNotPlayVideo %d, mNotPlayAudio %d",isAudioFlag,mExtractor->mNotPlayVideo,mExtractor->mNotPlayAudio);
                Mutex::Autolock autoLock(mMiscStateLock);
                ret = mExtractor->seek(seekTimeUs);
                if (ret != OK) {
                    ALOGE("read: mExtractor->seek() meet %s !", (ret==ERROR_END_OF_STREAM) ? "end of stream" : "error");
                    *out = NULL;
                    mExtractor->mSeekFailed = 1;
                    return ERROR_END_OF_STREAM;
                } else {
                    EXTRACT_LOGT("read: mExtractor->seek() ok");
                    mExtractor->mVideoAlreadySeek = 1;              
                }
            } else {              
                EXTRACT_LOGD("read: audio's seek demand ignored");
                //mExtractor->mVideoAlreadySeek = 0;      //for seek
                if (mExtractor->mSeekFailed == 1) {
                    ALOGE("read: video seek error, audio read no more response");                       
                    return ERROR_END_OF_STREAM;
                }
            }
        } else {
            EXTRACT_LOGD("mExtractor can not seek, continue playing...");
            seeked = false;
        }
    }

    {
    Mutex::Autolock autoLock(mMiscStateLock);
ReadAgain:
    av_buf_t av_buf={0};
    ret = mExtractor->getPacket(&av_buf, t);
    if (ret==ERROR_BUFFER_TOO_SMALL && mReadAgainCouter<1000) {
        usleep(10000);
        mReadAgainCouter++;
        goto ReadAgain;
    }
    if (OK != ret) {
        *out = NULL;
        ALOGE("mReadAgainCouter is %d \n",mReadAgainCouter);
        if (mReadAgainCouter==1000) {
            ret=ERROR_END_OF_STREAM;
        }
        if (ret!=INFO_NO_RAW_DATA_NOW) {
            ALOGW("Exit read: get Packet meet %s(%d) type: %s", 
                    (ret == ERROR_END_OF_STREAM) ? "end of stream" : 
                    ((ret==INFO_NO_RAW_DATA_NOW) ? "not find raw data" : "error"), ret,
                    (mTrackIndex == 0) ? "video" : "audio");
        }
        
        return (ret == ERROR_END_OF_STREAM) ? ERROR_END_OF_STREAM : 
                    ((ret==INFO_NO_RAW_DATA_NOW) ? INFO_NO_RAW_DATA_NOW : ret);
    }
    mReadAgainCouter=0;

    int offset = ((mExtractor->mEtm->ct == CODEC_CODA) || (isAudioFlag== true)) ? SIZE_PHT : 0;

    if ((av_buf.data == 0) || ((av_buf.data_len-offset) == 0)) {
        *out = NULL;
        ALOGE("Exit read => meet no av data error!!! ret(%d)", ret);
        return !OK;
    }
    
#if OUTPUT_PARSER_DATA_A
    if ((isAudioFlag== false) && (seeked == true)) {
        fwrite(av_buf.data + offset, 1, av_buf.data_len - offset, fp_audio_data);
    }
#endif

    packet_header_t *pht = (packet_header_t*)av_buf.data;   
#if 0 
    if (t == VIDEO_PACKET && mExtractor->m_vp.start_phyaddr != NULL) {
        actal_cache_flush(av_buf.data,av_buf.data_len);
}
#endif
    MediaBuffer *buffer = new MediaBuffer(av_buf.data + offset, av_buf.data_len - offset);

    int64_t pkttime = (int64_t)pht->packet_ts*1000;

    if (isAudioFlag== true) {
        if (mExtractor->mNotPlayAudio==false) {       
            if (seeked == true) {
                pkttime = (int64_t)mExtractor->getFirstAudioTime()*1000;
                EXTRACT_LOGV("read: seeked is %s and Audio's first packet time to %lld %lld %lld", 
                seeked == true ? "true": "false", pkttime, (int64_t)pht->packet_ts*1000, (int64_t)mExtractor->getFirstAudioTime()*1000);
            } else {
                EXTRACT_LOGV("read: seeked is %s and %s packet time to %lld", seeked == true ? "true": "false", (isAudioFlag == false) ? "video" : "audio",pkttime);
                if (mExtractor->mIsSetTrack==false) {
                    pkttime=0;
                } else {
                    mExtractor->mIsSetTrack = false;
                }
            }
        }
        if (mExtractor->mVideoAlreadySeek == 1) {
            buffer->meta_data()->setInt32(kKeyIsCodecConfig, 1);
            mExtractor->mVideoAlreadySeek = 0;
            EXTRACT_LOGD("read: set Audio pkt reset kKeyIsCodecConfig flag to 1 ");
        } else {
            buffer->meta_data()->setInt32(kKeyIsCodecConfig, 0);
        }
    }

    buffer->meta_data()->setInt64(kKeyTime, pkttime);
    buffer->meta_data()->setInt32(kKeyIsSyncFrame, 0);

    *out = buffer;
    EXTRACT_LOGV("exit read: %s pkttime: %lld", (isAudioFlag == false) ? "video" : "audio", pkttime);
    }
    return OK;
}

bool ActVideoExtractor::open_plugin(const char *ext)
{
#define LIBPOSTFIX ".so"
    typedef void *(* func_t)(void);
    char libname[32] = {'a', 'v', 'd', '_', '\0'};
    if (strlen(ext)< sizeof(m_ext)) {
        strcpy(m_ext,ext);
    } else {
        memcpy(m_ext, ext, sizeof(m_ext)-1);
        m_ext[sizeof(m_ext)-1] = '\0';
        ALOGE("ext(%s) too long, stripped to m_ext(%s)", ext, m_ext);
    }

    strcat(libname, ext);
    strcat(libname, LIBPOSTFIX);
    EXTRACT_LOGV("libname: %s", libname);
    mLib_handle = dlopen(libname, RTLD_NOW);
    if (mLib_handle == NULL) {
        ALOGE("dlopen err, libname: %s", libname);
        return false;
    }

    CHECK(mLib_handle!=NULL);
    func_t f = (func_t)dlsym(mLib_handle, "get_plugin_info");
    if (f == NULL) {
        ALOGE("dlsym err");
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return false;
    }

    m_dp = (demux_plugin_t*)f();
    if (m_dp == NULL) {
        ALOGE("get_plugin_info err");
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return false;
    }
    CHECK(m_dp!=NULL);
    return true;
#undef LIBPOSTFIX
}

void appendfile(const char* path, void* buf, size_t size)
{
    FILE* f;
    f = fopen(path, "a+");
    if (f == NULL) {
        ALOGE("appendfile: fopen %s error", path);
        return;
    }

    EXTRACT_LOGD("Appending to file %s, size(%d)", path, size);
    fwrite(buf, 1, size, f);
    fclose(f);
    return;
}

void output_mediabuffer(MediaBuffer *out, packet_type_t type)
{
    const char* path=NULL;
    switch (type) {
        case AUDIO_PACKET:
            path = "/data/a.raw";
            break;
        case VIDEO_PACKET:
            path = "/data/v.raw";
            break;
        case SUBPIC_PACKET:
            path = "/data/s.raw";
            break;
        default:
            path = "/data/bin.raw";
            break;
    }
    appendfile(path, out->data(), out->size());
    return;
}

void media_info_print(media_info_t *movi_info)
{
#define printf ALOGV
    printf("media info ....\n");
    printf("\tmedia_type:%d\n",movi_info->media_type);
    printf("\ttotal_time:%d\n",movi_info->total_time);
    printf("\taudio_num:%d\n",movi_info->audio_num);
    printf("\tsub_num:%d\n",movi_info->sub_num);

    printf("\twidth:%d\n",movi_info->parser_video.width);
    printf("\theight:%d\n",movi_info->parser_video.height);
    printf("\tframe_rate:%d\n",movi_info->parser_video.frame_rate);
    printf("\tsample_rate:%d\n",movi_info->parser_audio[0].sample_rate);
    printf("\tchannel:%d\n",movi_info->parser_audio[0].channels);
    printf("\tfirst_audio_time:%d\n",movi_info->first_audio_time);
    printf("\taudio:%s\n",movi_info->parser_audio[0].extension);
    printf("\tvideo:%s\n",movi_info->parser_video.extension);
    printf("\tindex_flag:%d\n",movi_info->index_flag);
    printf("\taudio_bitrate:%d\n",movi_info->parser_audio[0].audio_bitrate);
    printf("\tvideo_bitrate:%d\n",movi_info->parser_video.video_bitrate);
#undef printf
}

static void linear_buf_reset(stream_buf_t* t) {
    CHECK(t != NULL);
    t->empty.a = t->start;
    t->empty.b = t->start + t->size;
}

static bool linear_buf_allocate_phyCont(stream_buf_t* t, size_t size)
{
    int32_t phy_addr;
    t->start = (uint8_t*)actal_malloc_wt(size, &phy_addr);
    t->start_phyaddr=(uint8_t*)phy_addr;
    ALOGE("linear_buf_allocate_phyCont: size:%d, viraddr:0x%x,phyaddr:0x%x \n",size, (unsigned int)t->start,(unsigned int)t->start_phyaddr);

    if (t->start == NULL || t->start_phyaddr == NULL) {
        ALOGE("linear_buf_allocate_phyCont malloc err");
        return false;
    }
    if (size <= 14*1024*1024) {
        t->size = size;
    } else {
        t->size = size-3*1024*1024;
    }
    t->real_size = size;
    
    linear_buf_reset(t);

    ALOGE("linear_buf_allocate: start(%p), size(%d), empty.(a, b)(%p, %p)", t->start_phyaddr, t->size, t->empty.a, t->empty.b);

    return true;
}

static bool linear_buf_free_phyCont(stream_buf_t* t)
{
    if (t->start) {
        actal_free_wt(t->start);
    }

    ALOGE("linear buff free :0x%x", (unsigned int)t->start);

    t->start = NULL;
    t->start_phyaddr = NULL;
    t->size = 0;
    t->real_size=0;
    t->empty.a = NULL;
    t->empty.b = NULL;
    return true;
}

static bool linear_buf_allocate(stream_buf_t* t, size_t size)
{
    t->start = (uint8_t*)actal_malloc(size);
    t->start_phyaddr=NULL;

    if (t->start == NULL) {
        ALOGE("linear_buf_allocate: malloc(%d)  error", size);
        return false;
    }
    t->size = size;
    t->real_size = size;
    linear_buf_reset(t);
    BUFFER_LOGD("linear_buf_allocate: start(%x), size(%x), empty.a(%x), empty.b(%x)", (void*)t->start, t->size, (void*)t->empty.a, (void*)t->empty.b);
    return true;
}

static bool linear_buf_free(stream_buf_t* t)
{
    if (t->start) {
        actal_free(t->start);
    }
    t->start = NULL;
    t->size = 0;
    t->real_size=0;
    t->empty.a = NULL;
    t->empty.b = NULL;
    return true;
}

bool ActVideoExtractor::stream_buf_init()
{
    EXTRACT_LOGD("stream_buf_init: width: %d", m_mi.parser_video.width);
    uint32_t ss_a = 0;
    uint32_t ss_v = 0;
    uint32_t width=m_mi.parser_video.width;
    m_pkt_maxs.a = 180*1024;
    m_pkt_maxs.s = 20*1024;
#ifdef USE_PHY_CONTINUAL_STREAM_BUF
    if (m_cookie!=NULL) {
        if (!strncmp("rm", m_ext, 2) || !strncmp("ts", m_ext, 2)) {
            if (m_mi.parser_video.video_bitrate<8*1024*1024) {
                ss_v = 14*1024*1024;
            } else if (m_mi.parser_video.video_bitrate >= 8*1024*1024 && m_mi.parser_video.video_bitrate <= 20*1024*1024) {
                ss_v = 20*1024*1024;
            } else {
                ss_v = 30*1024*1024;
            }
        } else {
            if (m_mi.parser_video.video_bitrate < 20*1024*1024) {
                ss_v = 14*1024*1024;
            } else if (m_mi.parser_video.video_bitrate >= 20*1024*1024 && m_mi.parser_video.video_bitrate < 40*1024*1024) {
                ss_v = 20*1024*1024;
            } else {
                ss_v = 30*1024*1024;
            }
        }
        if (m_mi.parser_video.width > 2048 && ss_v < 25*1024*1024 ) {
            ss_v = 25*1024*1024;
        }
    } else {
        if(width > 1920) {
            ss_v = 12*1024*1024;
        } else {
            ss_v = 6*1024*1024;
        }
    }
#else
    ss_v = 70*1024*1024;
#endif
    ss_a = 12*1024*1024;
  
    if (width > 1920) {
        m_pkt_maxs.v = 3*1024*1024; 
    } else if (width > 1280) { 
        m_pkt_maxs.v = 2*1024*1024;    
    } else if (width >= 720) { 
        m_pkt_maxs.v = 800*1024;
    } else if(width > 352) {
        m_pkt_maxs.v = 230*1024;
    } else {   
        m_pkt_maxs.v = 80*1024;       
    }

    if (false == linear_buf_allocate(&m_ap, (size_t)ss_a)) {
        return false;
    }
    m_ap.real_size = ss_a;
    m_ap.size = ss_a -2*1024*1024;

#ifdef USE_PHY_CONTINUAL_STREAM_BUF
    mUsingPhyContBuffer = true;
#else
    mUsingPhyContBuffer = false;
#endif

    if (!strcmp(m_mi.parser_video.extension, "flv")) {
        mUsingPhyContBuffer = false; 
    }    

    if (mUsingPhyContBuffer) {
        if (false == linear_buf_allocate_phyCont(&m_vp, (size_t)ss_v)) {
            linear_buf_free(&m_ap);
            return false;
        }
    } else {         
        if (false == linear_buf_allocate(&m_vp, (size_t)ss_v)) {
            linear_buf_free(&m_ap);
            return false;
        }
    }        
    return true;
}

bool ActVideoExtractor::stream_buf_destroy()
{
    linear_buf_free(&m_ap);
    linear_buf_free(&m_ap_spare);
    linear_buf_free(&m_vp_spare);

    if (mUsingPhyContBuffer) {
        linear_buf_free_phyCont(&m_vp);     
    } else {
        linear_buf_free(&m_vp);
    }    
    return true;
}
void ActVideoExtractor::audio_stream_buf_reset()
{
    linear_buf_reset(&m_ap);
    linear_buf_reset(&m_ap_spare);
    List<av_buf_t>::iterator it = mQueue.begin();
    while (it != mQueue.end()) {
        packet_header_t *p = (packet_header_t*)(*it).data;
        if (p !=NULL ) {
            if (p->header_type==AUDIO_PACKET) {
                it = mQueue.erase(it);;
                continue;
            }           
        }
        it++;
    }
}

void ActVideoExtractor::stream_buf_reset()
{
    EXTRACT_LOGT("stream_buf_reset--SRT: m_ap(a: 0x%x b: 0x%x) m_vp(a: 0x%x b: 0x%x)", 
        m_ap.empty.a, m_ap.empty.b, m_vp.empty.a, m_vp.empty.b);
    linear_buf_reset(&m_ap);
    linear_buf_reset(&m_vp);
    linear_buf_reset(&m_ap_spare);
    linear_buf_reset(&m_vp_spare);

    mQueue.clear();
    EXTRACT_LOGT("stream_buf_reset--END: m_ap(a: 0x%x b: 0x%x) m_vp(a: 0x%x b: 0x%x)", 
        m_ap.empty.a, m_ap.empty.b, m_vp.empty.a, m_vp.empty.b);
}

bool ActVideoExtractor::stream_buf_empty_area_update(av_buf_t *p, bool enlarge)
{
    CHECK(p);
    packet_header_t *pht = (packet_header_t*)p->data;
    packet_type_t t = (packet_type_t)pht->header_type;
    stream_buf_t *s;
    if (t == AUDIO_PACKET) {
        if (mNotPlayAudio) {
            return true;
        }   
        if (mApUsingSpare== false) {
            s = &m_ap;
        } else {
            s = &m_ap_spare;
        } 
    } else if (t == VIDEO_PACKET) {
        if (mNotPlayVideo) {
            return true;
        }
        if (mVpUsingSpare== false) {
            s = &m_vp;
        } else {
            s = &m_vp_spare;

        }
    } else {
        ALOGE("stream_buf_empty_area_update: meet unknown packet type, t(0x%x), enlarge(%d)", t, enlarge);
        return false;
    }
    uint8_t** pp;
    if (enlarge == true) {
        pp = &s->empty.b;
    } else {
      pp = &s->empty.a;
    }

    *pp = p->data + ((p->data_len + (WORD_SIZE -1))&WORD_MASK);

    return true;
}

void ActVideoExtractor::stream_buf_status_dump(stream_buf_t* t)
{
    BUFFER_LOGD("********stream_buf_status_dump********");
    BUFFER_LOGD("stream_buf_t: start(%p), size(%d)", t->start, t->size);
    BUFFER_LOGD("    original region: (%p, %p)", t->start, t->start + t->size);
    BUFFER_LOGD("    now       empty: (%p, %p)", t->empty.a, t->empty.b);
    BUFFER_LOGD("mQueue.size() is %d", mQueue.size());

    List<av_buf_t>::iterator it = mQueue.begin();
    int i = 0;
    uint32_t used = 0;
    while (it != mQueue.end()) {
        packet_header_t *p = (packet_header_t*)(*it).data;
        BUFFER_LOGD("    %d-th: header_type(0x%x), area(%p, %p), av_buf(%p, %d)", \
                i++, (uint32_t)p->header_type, (*it).data, (*it).data + (*it).data_len, \
                (*it).data, (*it).data_len);
        used += (*it).data_len;
        it++;
    }
    BUFFER_LOGD("    Used buffer size is %d in sum",  used);
    BUFFER_LOGD("  You might need to increase stream buffer size or  just decrease the max packet size");
}

void ActVideoExtractor::stream_buf_quelst(packet_type_t type)
{
#if 0 // TURN_ON_STREAM_BUFFER_MANAGE_FLAG 
    List<av_buf_t>::iterator its = mQueue.begin();
    while (its != mQueue.end()) {
        CHECK((*its).data !=NULL);
        packet_header_t *p = (packet_header_t*)(*its).data;
        if (p->header_type == (uint32_t)type) {
            ALOGW("stream_buf_quelst: %s  Queue list! data:0x%x data_len: 0x%06x ", 
            (type==VIDEO_PACKET) ? "video packet" : "audio packet", (uint32_t)(*its).data, (*its).data_len);
        }
        its++;
    }
#endif
}

bool ActVideoExtractor::stream_buf_assign(packet_type_t type, av_buf_t* av_buf)
{
    EXTRACT_LOGV("entry stream_buf_assign");
    stream_buf_t* t=NULL;
    stream_buf_t* t_spare=NULL;
    uint32_t demanded=0;
    switch (type) {
        case AUDIO_PACKET:
            t = &m_ap;
            demanded = m_pkt_maxs.a;
            break;
        case VIDEO_PACKET:
            t = &m_vp;
            demanded = m_pkt_maxs.v;
            break;
        default:
            ALOGE("Bad packet type(0x%x)!", type);
            return false;
    }

    if (t->empty.b > t->empty.a)
    {
        size_t left = t->empty.b - t->empty.a;
        if (demanded > left) {
            if (type == VIDEO_PACKET) {
                if (m_vp_spare.start == NULL) {
                    if (false == linear_buf_allocate(&m_vp_spare, (m_vp.size)<<1)) {
                        return false;
                    }
                }
                t_spare=&m_vp_spare;
                mVpUsingSpare = true;
            } else {
                if (m_ap_spare.start == NULL) {
                    if (false == linear_buf_allocate(&m_ap_spare, m_ap.size)) {
                        return false;
                    }
                }
                t_spare=&m_ap_spare;
                mApUsingSpare =true;
            }
            mUsingSpare = true;

            if (t_spare->empty.b > t_spare->empty.a) {
                size_t left = t_spare->empty.b - t_spare->empty.a;
                if (demanded>left) {
                    EXTRACT_LOGV("stream_buf_assign: %s Fatal error, stream-buf full --0! demanded(0x%06x) left(0x%06x), start(0x%x), size(0x%06x), a(0x%x), b(0x%x)",\
                    (type==VIDEO_PACKET) ? "video packet" : "audio packet",demanded, left, (uint32_t)t->start, t->size, 
                    (uint32_t)t_spare->empty.a, (uint32_t)t_spare->empty.b);
                    stream_buf_status_dump(t_spare);
                    return false;
                }
            } else if (t_spare->empty.b < t_spare->empty.a) {
                size_t left = t_spare->start + t_spare->size - t_spare->empty.a;
                if (demanded > left) {
                    t_spare->empty.a = t_spare->start;
                    left = t_spare->empty.b - t_spare->empty.a;
                    if (demanded > left) {
                        EXTRACT_LOGV("stream_buf_assign: %s Fatal error, stream-buf full--1! demanded(0x%06x) left(0x%06x), start(0x%x), size(0x%06x), a(0x%x), b(0x%x)",\
                        (type==VIDEO_PACKET) ? "video packet" : "audio packet",demanded, left, (uint32_t)t->start, t->size, 
                        (uint32_t)t_spare->empty.a, (uint32_t)t_spare->empty.b);
                        stream_buf_status_dump(t_spare);
                        return false;
                    }
                }
            }
            av_buf->data = t_spare->empty.a;

            av_buf->data_len = 0;
            return true;
        }
    } else if (t->empty.b < t->empty.a) {
        size_t left = t->start + t->size - t->empty.a;
        if (demanded > left) {
            t->empty.a = t->start;
            left = t->empty.b - t->empty.a;
            if  (demanded > left) {
                if (type == VIDEO_PACKET) {
                    if (m_vp_spare.start == NULL) {
                        if (false == linear_buf_allocate(&m_vp_spare, (m_vp.size)<<1)) {
                            return false;
                        }
                    }
                    t_spare=&m_vp_spare;
                    mVpUsingSpare =true;
                } else {
                    if (m_ap_spare.start == NULL) {
                        if (false == linear_buf_allocate(&m_ap_spare, m_ap.size)) {
                            return false;
                        }
                    }
                    t_spare=&m_ap_spare;
                    mApUsingSpare =true;
                }
                mUsingSpare=true;
                if (t_spare->empty.b > t_spare->empty.a) {
                    size_t left = t_spare->empty.b - t_spare->empty.a;
                    if (demanded>left) {
                        EXTRACT_LOGV("stream_buf_assign: %s Fatal error, stream-buf full --0! demanded(0x%06x) left(0x%06x), start(0x%x), size(0x%06x), a(0x%x), b(0x%x)",\
                        (type==VIDEO_PACKET) ? "video packet" : "audio packet",demanded, left, (uint32_t)t->start, t->size, 
                        (uint32_t)t_spare->empty.a, (uint32_t)t_spare->empty.b);
                        stream_buf_status_dump(t_spare);
                        return false;
                    }
                } else if (t_spare->empty.b < t_spare->empty.a) {
                    size_t left = t_spare->start + t_spare->size - t_spare->empty.a;
                    if (demanded > left) {
                        t_spare->empty.a = t_spare->start;
                        left = t_spare->empty.b - t_spare->empty.a;
                        if (demanded > left) {
                            EXTRACT_LOGV("stream_buf_assign: %s Fatal error, stream-buf full--1! demanded(0x%06x) left(0x%06x), start(0x%x), size(0x%06x), a(0x%x), b(0x%x)",\
                            (type==VIDEO_PACKET) ? "video packet" : "audio packet",demanded, left, (uint32_t)t->start, t->size, 
                            (uint32_t)t_spare->empty.a, (uint32_t)t_spare->empty.b);
                            stream_buf_status_dump(t_spare);
                            return false;
                        }
                    }
                }
                av_buf->data = t_spare->empty.a;
                av_buf->data_len = 0;
                return true;
            }
        }
    } else if (t->empty.b == t->empty.a) {
        int anum = 0;
        int vnum = 0;

        List<av_buf_t>::iterator it = mQueue.begin();
        while (it != mQueue.end()) {
            packet_header_t *p = (packet_header_t*)(*it).data;
            if (p->header_type == AUDIO_PACKET) {
                anum++;
            } else if (p->header_type == VIDEO_PACKET) {
                vnum++;
            } else {
                EXTRACT_LOGV("This frake!");
                stream_buf_status_dump(t);
                return false;
            }
            it++;
        }

        if (((type == AUDIO_PACKET) && (anum != 0)) ||((type == VIDEO_PACKET) && (vnum != 0))) {
            ALOGE("stream_buf_assign: Error, %d demanded while 0 left..", demanded);
            stream_buf_status_dump(t);
            return false;
        }
        
#if 1       
        size_t left = t->start + t->size - t->empty.a;
        size_t left1 = t->empty.a - t->start; 
        
        if (demanded > left) {
            //ALOGD("cathylogs: the backwork part is not enough ! (demanded: 0x%x > left: 0x%x) : %s", demanded, left, (demanded > left) ? "yes" : "no");
            linear_buf_reset(t);
        } else if (demanded > left1){
            //ALOGD("cathylogs: the forwaord part is not enough ! (demanded: 0x%x > left1: 0x%x) : %s", demanded, left1, (demanded > left1) ? "yes" : "no");
        } else {
        }               
#endif
    }

    av_buf->data = t->empty.a;
    av_buf->data_len = 0;
    EXTRACT_LOGV("stream_buf_assign: %s start: 0x%x empty.a: 0x%x empty.b: 0x%x data: 0x%x demanded: 0x%06x end: 0x%x",
                (type==VIDEO_PACKET) ? "video packet" : "audio packet",(uint32_t)t->start,  
                (uint32_t)t->empty.a, (uint32_t)t->empty.b, (uint32_t)av_buf->data, demanded, 
                (uint32_t)t->start + t->size);
    EXTRACT_LOGV("exit stream_buf_assign");
    return true;
}

bool ActVideoExtractor::stream_buf_check_overflow(av_buf_t *av_buf)
{
    CHECK(av_buf);
    uint8_t* p = av_buf->data;
    uint32_t len = av_buf->data_len;
    stream_buf_t *s=NULL;
    uint32_t pkt_maxs = 0;

    packet_header_t *pht = (packet_header_t*)p;
    packet_type_t t = (packet_type_t)pht->header_type;

    if (t == AUDIO_PACKET) {
        if (mApUsingSpare == false) {
            s = &m_ap;
        } else {
            s = &m_ap_spare;
        } 
        pkt_maxs = m_pkt_maxs.a;
    } else if (t == VIDEO_PACKET) {
        if (mVpUsingSpare == false) {
            s = &m_vp;
        } else {
            s = &m_vp_spare;
        } 
        pkt_maxs = m_pkt_maxs.v;
    } else {
        ALOGE("stream_buf_check_overflow: Check this, t(0x%x)", t);
        return false;
    }

    if (len > pkt_maxs) {
        ALOGE("stream_buf_check_overflow: Check this, packet(%d) exceeds packet_maxs(%d).", len, pkt_maxs);
    }
    if ((p < s->empty.b) && ((p + len) > s->empty.b)) {
        ALOGE("stream_buf_check_overflow: Fatal, stream-buf overlapped! %p+%d > %p", p, len, s->empty.b);
        stream_buf_status_dump(s);
        return false;
    } else if ((p + len) > (s->start + s->size)) {
        ALOGE("stream_buf_check_overflow: Fatal, stream-buf overflow! %p+%d >%p", p, len, s->start + s->size);
        stream_buf_status_dump(s);
        return false;
    }

    return true;
}

bool ActVideoExtractor::stream_buf_prepare(av_buf_t *ao_buf, av_buf_t *vo_buf)
{
    ao_buf->data_len = 0;
    vo_buf->data_len = 0;
    mUsingSpare = false;
    mVpUsingSpare =false;
    mApUsingSpare =false;
    if (false == stream_buf_assign(AUDIO_PACKET, ao_buf)) {
        stream_buf_quelst(AUDIO_PACKET);
        EXTRACT_LOGV("stream_buf_prepare() => stream_buf_assign audio packet error ");
        return false;
    }
    if (false == stream_buf_assign(VIDEO_PACKET, vo_buf)) {
        stream_buf_quelst(VIDEO_PACKET);
        EXTRACT_LOGV("stream_buf_prepare() => stream_buf_assign video packet error ");
        return false;
    }
    EXTRACT_LOGV("exit stream_buf_prepare");
    return true;
}

void ActVideoExtractor::avbufItemAdd(av_buf_t* av_buf)
{
    EXTRACT_LOGV("entry avbufItemAdd");
    CHECK(av_buf);
    av_buf_t tmp = *av_buf;
    //ALOGE("===add:%x==\n",av_buf->data);
    mQueue.push_back(tmp);
    stream_buf_empty_area_update(av_buf, false);
    EXTRACT_LOGV("exit avbufItemAdd");
}

bool ActVideoExtractor::avbufItemCheck()
{
    List<av_buf_t>::iterator it = mQueue.begin();
    bool notruined = true;
    while (it != mQueue.end()) {
        packet_header_t *p = (packet_header_t*)(*it).data;
        if (p->header_type != AUDIO_PACKET && p->header_type != VIDEO_PACKET) {
            ALOGE("avbufItemCheck: find !video and !audio packet info, error!!!%p, %d, 0x%x", (*it).data, (*it).data_len, p->header_type);
            if (notruined == true) {
                stream_buf_status_dump(&m_ap);
                stream_buf_status_dump(&m_vp);
            }
            notruined = false;
            /* remove this packet from stream buffer */
            av_buf_t tmp;
            tmp.data = (*it).data;
            tmp.data_len = (*it).data_len;
            mQueue.erase(it);
            stream_buf_empty_area_update(&tmp, true);
        }
        it++;
    }

    return notruined;
}

bool ActVideoExtractor::avbufItemPick(packet_type_t t, av_buf_t* av_buf) 
{
    CHECK(av_buf);
    int32_t audio_pkt_num = 0;
    int32_t video_pkt_num = 0;
    List<av_buf_t>::iterator it = mQueue.begin();
    mVpUsingSpare = false;
    mApUsingSpare = false;
    
#if 1
    mCounter++;
    if ((mVDuration - mVLastPktTime) >= DIFFTOEND) {
        if (t == VIDEO_PACKET && PLUGIN_RETURN_NORMAL == m_pstatus && m_cookie != NULL) {
            while (it != mQueue.end()) {
                packet_header_t *p = (packet_header_t*)(*it).data;
                if (p->header_type ==VIDEO_PACKET) {
                    video_pkt_num++;
                } else if (p->header_type ==AUDIO_PACKET) {
                    audio_pkt_num++;
                }
                it++;
            }
        it = mQueue.begin();
        if (m_vp.empty.b > m_vp.empty.a) {
            if (((m_vp.empty.b-m_vp.empty.a) > m_vp.real_size/6 && video_pkt_num < m_mi.parser_video.frame_rate && mCounter < 2 && m_vp_spare.start == NULL) || (mCounter<2 && m_vp_spare.start == NULL && audio_pkt_num < 1 && (m_vp.empty.b-m_vp.empty.a) > m_vp.real_size/6)) {
                return false;
            }
        } else if (m_vp.empty.b < m_vp.empty.a) {
            if (((m_vp.empty.a-m_vp.empty.b) < 5*m_vp.real_size/6 && video_pkt_num<m_mi.parser_video.frame_rate && mCounter < 2 && m_vp_spare.start == NULL) || (mCounter < 2 && m_vp_spare.start == NULL && audio_pkt_num < 1 && (m_vp.empty.a-m_vp.empty.b) < 5*m_vp.real_size/6)) {
                    return false;
                }
            }
        }
    }
#endif

    while (it != mQueue.end()) {
        CHECK((*it).data !=NULL);
        packet_header_t *p = (packet_header_t*)(*it).data;
        if (p->header_type == (uint32_t)t) {
            if (t == VIDEO_PACKET && (*it).data >= m_vp_spare.start && (*it).data < 
                m_vp_spare.start + m_vp_spare.size) {
                    size_t left; 
                    mUsingSpare = true;
                    mVpUsingSpare = true;
                    if (mVpSpareData == NULL) {
                        if (m_vp.real_size>m_vp.size) {
                            mVpSpareData = m_vp.start + m_vp.size;
                        } else {
                            mVpSpareData = m_vp.start;
                        }
                    } 
                    left = m_vp.start + m_vp.real_size - mVpSpareData;
                    if (left<(((*it).data_len + (WORD_SIZE -1))&WORD_MASK)) {
                        EXTRACT_LOGV("real_size:%d,size:%d \n",m_vp.real_size,m_vp.size);
                        if (m_vp.real_size>m_vp.size) {
                            mVpSpareData = m_vp.start + m_vp.size;
                        } else{
                            mVpSpareData = m_vp.start;
                        }
                    }
                    av_buf->data = (*it).data;
                    av_buf->data_len = (*it).data_len;
                    mQueue.erase(it);
                    stream_buf_empty_area_update(av_buf, true); 
                    actal_memcpy(mVpSpareData,av_buf->data,av_buf->data_len);  
                    av_buf->data = mVpSpareData;  
                    mVpSpareData += ((av_buf->data_len + (WORD_SIZE -1))&WORD_MASK);
                } else if (t == AUDIO_PACKET && (*it).data >= m_ap_spare.start && (*it).data < m_ap_spare.start + m_ap_spare.size)
                {
                    size_t left; 
                    mApUsingSpare = true;
                    if (mApSpareData == NULL) {
                        if (m_ap.real_size>m_ap.size) {
                            mApSpareData = m_ap.start + m_ap.size;
                        } else {
                            mApSpareData = m_ap.start;
                        }
                    }
                    left = m_ap.start + m_ap.real_size - mApSpareData;
                    if (left < (((*it).data_len + (WORD_SIZE -1)) & WORD_MASK)) {
                        if (m_ap.real_size > m_ap.size) {
                            mApSpareData = m_ap.start + m_ap.size;
                        } else {
                            mApSpareData = m_ap.start;
                        }
                    }
                    av_buf->data = (*it).data;
                    av_buf->data_len = (*it).data_len;
                    mQueue.erase(it);
                    stream_buf_empty_area_update(av_buf, true); 
                    actal_memcpy(mApSpareData,av_buf->data,av_buf->data_len);  
                    av_buf->data = mApSpareData;  
                    mApSpareData += ((av_buf->data_len + (WORD_SIZE -1))&WORD_MASK);
                } else {
                    av_buf->data = (*it).data;          
                    av_buf->data_len = (*it).data_len;
                    mQueue.erase(it);
                    stream_buf_empty_area_update(av_buf, true);
                }
                EXTRACT_LOGV("exit avbufItemPick");
                return true;
            }
            it++;
    }

    EXTRACT_LOGV("exit avbufItemPick false");
    return false;
}

packet_type_t ActVideoExtractor::matchPacketType(size_t track_idx)
{
    Mutex::Autolock autoLock(mLock);
    EXTRACT_LOGV("entry matchPacketType");
    packet_type_t t=(packet_type_t)UNKNOWN_PACKET;
    if (track_idx == m_ti_playing.v) {
        t = VIDEO_PACKET;
    } else if (track_idx == m_ti_playing.a) {
        t = AUDIO_PACKET;
    } else {
        t = (packet_type_t)UNKNOWN_PACKET;
        ALOGE("matchPacketType() => track_idx (%d) UNKNOWN_PACKET", track_idx);
    }
    EXTRACT_LOGV("exit matchPacketType");
    return t;
}

status_t ActVideoExtractor::notifyEvent(EVENT_T msg, int ext1, int ext2)
{
    switch (msg) {
        case EXTRACTOR_SUBMIT_SUB_BUF: 
            {
                SUB_LOGD("EXTRACTOR_SUBMIT_SUB_BUF with m_cookie(%p)", m_cookie);
                av_buf_t *ppkt = (av_buf_t*) ext1;

                if (m_cookie) {
                    AwesomePlayer* p = reinterpret_cast<AwesomePlayer*> (m_cookie);
                    //appendfile("/data/sori.raw", ppkt->data, ppkt->data_len);
                    #ifdef SUB_ENABLE //disable now
                    p->notifyMe(MEDIA_SUB, (int) (ppkt->data), ext2/*ppkt->data_len*/); /* FIXME */
                    #endif
                } else {
                    ALOGE("m_cookie = NULL!");
                }
                break;
            }
        default:
            break;
    }

    return OK;
}

/*
 * retval:
 * true
 *    m_out_buf fills the av_buf's packet!
 * else
 *    m_out_buf be invalid
 */
status_t ActVideoExtractor::getPacket(av_buf_t* av_buf, packet_type_t t)
{
    Mutex::Autolock autoLock(mLock);
    av_buf->data = NULL;
    av_buf->data_len = 0;
    //m_pstatus = PLUGIN_RETURN_NORMAL;
    int32_t loop_times=0;
    mCounter=0;
    while (1) {
        {
            loop_times ++;
            Mutex::Autolock autoLock(mMiscStateLock);
            if (true == avbufItemPick(t, av_buf)) {
                /* a priviously parsed data be found  most recently used stream data */
                if (t == AUDIO_PACKET) {
                    m_ao_buf = *av_buf;
                } else if (t == VIDEO_PACKET) {
                    //m_vo_buf = *av_buf;
                } else {
                    ALOGE("getPacket: Check this, t(0x%x)", t);
                    return MEDIA_ERROR_BASE;
                }
                return OK;
            } else if (PLUGIN_RETURN_FILE_END == m_pstatus) {  /* file ended and fifo ended */
                ALOGW("getPacket: fifo ended(To file ended)");
                return ERROR_END_OF_STREAM;
            } else {
                if (loop_times > MAX_GET_PACKETS_LOOP_TIMES) {
                    //ALOGW("getPacket: %s packet meet INFO_NO_RAW_DATA_NOW after find %d packets TID: %d", 
                    //(t == VIDEO_PACKET) ? "vidieo" : "audio", loop_times, gettid());
                    ALOGE("===loop exceed max times===\n");
                    return ERROR_END_OF_STREAM;
                    //return INFO_NO_RAW_DATA_NOW;
                }
            }
        }
        av_buf_t ao_buf;
        av_buf_t vo_buf;
        ao_buf.data = NULL;
        ao_buf.data_len = 0;
        vo_buf.data = NULL;
        vo_buf.data_len = 0;
        if (false == stream_buf_prepare(&ao_buf, &vo_buf)) {
            EXTRACT_LOGV("getPacket: stream_buf_prepare() BUFFER_TOO_SMALL error!");
            return ERROR_BUFFER_TOO_SMALL;
        }
        {
            Mutex::Autolock autoLock(mMiscStateLock);

            m_pstatus = (plugin_err_no_t)m_dp->parse_stream(m_dh, &ao_buf, &vo_buf);

            if (vo_buf.data_len) {
                packet_header_t *pht = (packet_header_t*)vo_buf.data;
                mVLastPktTime = (int64_t)pht->packet_ts * 1000;
                if (pht->header_type == VIDEO_PACKET) {
                    m_vo_buf = vo_buf;
                }
            } else if (ao_buf.data_len) {
                packet_header_t *pht = (packet_header_t*)ao_buf.data;
                mALastPktTime = (int64_t)pht->packet_ts * 1000;             
            }
        }

        if ((PLUGIN_RETURN_NORMAL != m_pstatus)&&(PLUGIN_RETURN_FILE_END != m_pstatus)) {
            if (((mVDuration - mVLastPktTime) <= MINIMUM_LASTPK_TIMEDIFF) || ((mADuration - mALastPktTime) <= MINIMUM_LASTPK_TIMEDIFF)) {
                ALOGW("getPacket: parse end of file m_pstatus: %d mVDuration: %lld mADuration: %lld mVLastPktTime: %lld mALastPktTime: %lld", 
                            m_pstatus, mVDuration, mADuration, mVLastPktTime, mALastPktTime);
                return ERROR_END_OF_STREAM; 
            } else {
                ALOGE("getPacket: parse_stream()  error !!!m_pstatus: %d mVDuration: %lld mADuration: %lld mVLastPktTime: %lld mALastPktTime: %lld", 
                            m_pstatus, mVDuration, mADuration, mVLastPktTime, mALastPktTime);
                return MEDIA_ERROR_BASE;
            }
        }

        if ((ao_buf.data_len == 0) && (vo_buf.data_len == 0)) {
            if (PLUGIN_RETURN_FILE_END == m_pstatus) {
                ALOGE("getPacket: parser return fifo ended(To file ended)");
                return ERROR_END_OF_STREAM;
            }
            continue;
        }
        av_buf_t insert;
        if (vo_buf.data_len) {
            Mutex::Autolock autoLock(mMiscStateLock);
            insert = vo_buf;
            if (insert.data_len<=SIZE_PHT) {
                if (PLUGIN_RETURN_FILE_END == m_pstatus) {
                    ALOGE("getPacket: parser return fifo ended(size: %d) && no av data!!!", insert.data_len);
                    return ERROR_END_OF_STREAM;
                }
                continue;
            }
        
            CHECK(insert.data_len>SIZE_PHT);
            packet_header_t *pht = (packet_header_t*)insert.data;
#ifdef MMM_ENABLE_SUBTITLE              
            if (vo_buf.data_len != 0)  {
                packet_header_t *packet_header = (packet_header_t *)vo_buf.data;
                //SUB_LOGD("header_type %x",packet_header->header_type);
                if (packet_header->header_type < SUBPIC_PACKET)  {           
                    if (put_subtitle_packet_flag == 1) {
                        if (packet_header_org != NULL) {
                            //SUB_LOGD("0 input %x\n",packet_header_org);
                            SUB_LOGD("0 str is %s\n",((char *)packet_header_org+28));
                            SUB_LOGD("0 reserved_count %d\n",reserved_count);                                   
                            packet_header_org->reserved2 = reserved_count;
                            SUB_LOGD("0 packet_header_org->reserved2 = %d",packet_header_org->reserved2);
                            subtitle_buf_No_0++;
                            if (subtitle_buf_No_0 == MMM_SUBTITLE_NUM) {
                                subtitle_buf_No_0 = 0;
                            }
                            SUB_LOGD("0 memset, subtitle_buf_No_0 = %d",subtitle_buf_No_0);
                            actal_memset(subtitle_data[subtitle_buf_No_0],0,MMM_SUBTITLE_DATA_LEN);
                        }
                        put_subtitle_packet_flag = 0;
                    }
                } else {               
                    if (vo_buf.data_len>MMM_SUBTITLE_DATA_LEN) {
                        ALOGE("%s %d over!!\n",__FILE__,__LINE__);
                    }
                    SUB_LOGD("pkt_prv_ts %d, ts %d\n",pkt_prv_ts,packet_header->packet_ts);                    
                    if ((unsigned int)pkt_prv_ts != packet_header->packet_ts) {
                        if (put_subtitle_packet_flag == 1) {
                            if (packet_header_org != NULL) {
                                //SUB_LOGD("1 input %x\n",packet_header_org);
                                SUB_LOGD("1 str is %s\n",((char *)packet_header_org+28));
                                packet_header_org->reserved2 = reserved_count;
                                SUB_LOGD("1 packet_header_org->reserved2 = %d",packet_header_org->reserved2);       
                                subtitle_buf_No_0++;
                                if (subtitle_buf_No_0 == MMM_SUBTITLE_NUM) {
                                    subtitle_buf_No_0 = 0;
                                }
                                SUB_LOGD("1 memset, subtitle_buf_No_0 = %d",subtitle_buf_No_0);
                                actal_memset(subtitle_data[subtitle_buf_No_0],0,MMM_SUBTITLE_DATA_LEN);
                            }
                            put_subtitle_packet_flag = 0;
                        }
                        put_subtitle_packet_flag = 1;
                        pkt_prv_ts = packet_header->packet_ts;
                        SUB_LOGD("pkt len %d, pkt_prv_ts %d\n",vo_buf.data_len,pkt_prv_ts);
                        SUB_LOGD("2 memecpy, subtitle_buf_No_0 = %d, len = %d",subtitle_buf_No_0,vo_buf.data_len);
                        actal_memcpy(subtitle_data[subtitle_buf_No_0],vo_buf.data,vo_buf.data_len);
                        SUB_LOGD("2 str is %*.*s\n",vo_buf.data_len-28,vo_buf.data_len-28,(vo_buf.data+28));
                        packet_header_tmp = (packet_header_t *)(subtitle_data[subtitle_buf_No_0]);
                        packet_header_org = packet_header_tmp;  
                        SUB_LOGD("2 packet_header_org->reserved2 = 0");
                        packet_header_org->reserved2 = 0;
                        addr_oft = 0;
                        SUB_LOGD("2 reserved_count = 1");
                        reserved_count = 1;
                    } else {   
                        addr_oft += (packet_header_tmp->block_len + 28 +3)&(~3);                        
                        reserved_count++;
                        SUB_LOGD("3 reserved_count = %d",reserved_count);
                        if (addr_oft + vo_buf.data_len>MMM_SUBTITLE_DATA_LEN) {
                            actal_error("%s %d over!!\n",__FILE__,__LINE__);
                        }
                        //SUB_LOGD("addr_oft %d",addr_oft);
                        SUB_LOGD("3 str is %*.*s\n",vo_buf.data_len-28,vo_buf.data_len-28,(vo_buf.data+28));
                        SUB_LOGD("3 memecpy, subtitle_buf_No_0 = %d, len = %d",subtitle_buf_No_0,vo_buf.data_len);
                        actal_memcpy((subtitle_data[subtitle_buf_No_0] + addr_oft),vo_buf.data,vo_buf.data_len);
                        packet_header_tmp = (packet_header_t *)(subtitle_data[subtitle_buf_No_0] + addr_oft);
                        SUB_LOGD("now data len %d\n",(addr_oft+vo_buf.data_len));
                    }
                }
            }
#endif
            //mVLastPktTime = (int64_t)pht->packet_ts * 1000;
            if (pht->header_type == SUBPIC_PACKET) {
                notifyEvent(EXTRACTOR_SUBMIT_SUB_BUF, (int)&insert, 0);         
            } else {
                if (false == stream_buf_check_overflow(&insert)) {
                    ALOGE("getPacket: stream_buf_check_overflow() video buffer too small(size: %d)!!!", insert.data_len);
                    return ERROR_BUFFER_TOO_SMALL;
                }
                EXTRACT_LOGD("getPacket: add video packet");
                avbufItemAdd(&insert);
            }
        }
    
        if (ao_buf.data_len) {
            Mutex::Autolock autoLock(mMiscStateLock);
            insert = ao_buf;
            if (insert.data_len<=SIZE_PHT) {
                if (PLUGIN_RETURN_FILE_END == m_pstatus) {
                    ALOGE("getPacket: parser return fifo ended(size: %d) && no av data!!!", insert.data_len);
                    return ERROR_END_OF_STREAM;
                }
                continue;
            }
            CHECK(insert.data_len>SIZE_PHT);
            packet_header_t *pht = (packet_header_t*)insert.data;
            //mALastPktTime = (int64_t)pht->packet_ts * 1000;
            if (pht->header_type == SUBPIC_PACKET) {             
                //notifyEvent(EXTRACTOR_SUBMIT_SUB_BUF, (int)&insert, 0);           
            } else {
                if (false == stream_buf_check_overflow(&insert)) {
                    ALOGE("getPacket: stream_buf_check_overflow() audio buffer too small(size: %d)!!!", insert.data_len);
                    return ERROR_BUFFER_TOO_SMALL;
                }
                if (pht->header_type == AUDIO_PACKET) {
                    EXTRACT_LOGD("getPacket: add audio packet mAlwaysDropAudio: %d", mAlwaysDropAudio);
                    if (mAlwaysDropAudio == 0) {
                        avbufItemAdd(&insert);
                    }
                }
            }
        }
    }
    EXTRACT_LOGV("exit getPacket");
    return OK;
}

bool ActVideoExtractor::test()
{
    sp<ActVideoSource>* p;
    sp<ActVideoSource> v;
    sp<ActVideoSource> a;
    int64_t at = 0;
    int64_t vt = 0;
    MediaBuffer *out;
    MediaSource::ReadOptions options;
    bool rt = true;

/* p->buf */
#if 0
    if (m_mi.parser_audio[0].buf) {
        out = new MediaBuffer(m_mi.parser_audio[0].buf + SIZE_PHT, 12);
        output_mediabuffer(out, AUDIO_PACKET);
    }
#endif
    EXTRACT_LOGD("Now playing: v(%d), a(%d), s(%d)", m_ti_playing.v, m_ti_playing.a, m_ti_playing.s);
    v = new ActVideoSource(this, m_ti_playing.v);
    a = new ActVideoSource(this, m_ti_playing.a);

    while (1) {
        EXTRACT_LOGD("Audio/Video time(%d/%d)", (uint32_t)at, (uint32_t)vt);
        if (at < vt) {
            rt = a->read(&out, &options);
            if (rt != OK) {
                ALOGE("test: call audio read() return error: %d", rt);
                break;
            }
            out->meta_data()->findInt64(kKeyTime, &at);
            EXTRACT_LOGD("Audio time(%d)", (uint32_t)at);
            output_mediabuffer(out, AUDIO_PACKET);
        } else {
            rt = v->read(&out, &options);
            if (rt != OK) {
                ALOGE("test: call video read() return error: %d", rt);
                break;
            }
            out->meta_data()->findInt64(kKeyTime, &vt);
            EXTRACT_LOGD("Video time(%d)", (uint32_t)vt);
            output_mediabuffer(out, VIDEO_PACKET);
        }
    }
    m_input->seek(m_input, 0, DSEEK_SET);

    return true;
}

ActVideoExtractor::ActVideoExtractor(const sp<DataSource> &source, const char* mime, void* cookie)
    : m_dh(NULL),
      m_dp(NULL),
      m_input(NULL),
      m_cookie(cookie),
      m_pstatus(PLUGIN_RETURN_NORMAL),
      m_fstatus(true),
      mExtractorFlags(CAN_PAUSE),
      mLib_handle(NULL),
      mEtm(&kExtToMime[0]),
      mAEtm(&kAExtToMime[0]),
      mNotPlayVideo(false),
      mNotPlayAudio(false),
      mUsingSpare(false),
      mApUsingSpare(false),
      mVpUsingSpare(false),
      mIsSetTrack(false),
      mVpSpareData(NULL),
      mApSpareData(NULL),
      mUnsupportAudioTrackNum(0),
      mVLastPktTime(0),
      mADuration(0),
      m_exist_location(false),
      mALastPktTime(0),
      mVDuration(0)
{
    mQueue.clear();
    ALOGE("===mime is %s====\n",mime);
    actal_memset(&m_vp,0,sizeof(stream_buf_t));
    actal_memset(&m_vp_spare,0,sizeof(stream_buf_t));
    actal_memset(&m_ap,0,sizeof(stream_buf_t));
    actal_memset(&m_ap_spare,0,sizeof(stream_buf_t));
    m_input = stream_input_open();
    if (!m_input) {
        ALOGE("ActVideoExtractor() => stream_input_open() error !!!");
        return;
    }

    CHECK(m_input!=NULL);
    if (false == stream_input_init(m_input, source)) {
        ALOGE("ActVideoExtractor() => stream_input_init() error!");
        //return;
    }

    if (false == open_plugin(mime)) {
        ALOGE("ActVideoExtractor() => open_plugin() error !!!");
         if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return;
    }

    memset(&m_mi, 0, sizeof(media_info_t));
    m_dh = m_dp->open((void*)m_input, &m_mi);
    if (!m_dh) {
        ALOGE("ActVideoExtractor() => demuxer open(%p)  error && maybe unsupport stream!!!", m_dh);
         if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return;
    }
    CHECK(m_dh!=NULL);
    
    if (strncmp("mp4", m_ext, 3) == 0) {
        m_dp->ex_ops(m_dh,SET_SEEK_OPTIONS,2);
        if (m_cookie != NULL) {
            AwesomePlayer* mplayer = reinterpret_cast < AwesomePlayer* > (m_cookie);
            if (mplayer->getStreamingFlag() == true) {
                m_dp->ex_ops(m_dh,SET_SEEK_OPTIONS,1);
            }
        }
    }
  
    if (false == stream_buf_init()) {
        ALOGE("ActVideoExtractor() => stream_buf_init() error!");
        m_dp->dispose(m_dh);
        m_dp = NULL;
        m_dh = NULL;
        if (mLib_handle) {
            dlclose( mLib_handle);
            mLib_handle = NULL;
        }
        return;
    }

    addTracks();
    if (mNotPlayVideo == false && (m_mi.parser_video.width == 0 || m_mi.parser_video.height == 0)){
        return;
    }

    media_info_print(&m_mi);
    
    /* init m_ao_buf and m_vo_buf for setaudio track */
    m_ao_buf.data=NULL;
    m_ao_buf.data_len = 0;
    m_vo_buf.data=NULL;
    m_vo_buf.data_len = 0;
    //test();
    mSeekFailed = 0;
    mAlwaysDropAudio = 0; 
    #if ONLY_PLAY_VIDEO_FLAG
    mNotPlayAudio = true;
    #endif
    #if ONLY_PLAY_AUDIO_FLAG
    mNotPlayVideo = true;
    #endif
}

ActVideoExtractor::~ActVideoExtractor()
{
    EXTRACT_LOGV("~ActVideoExtractor()");
    if (m_dh) {
        m_dp->dispose(m_dh);
        m_dp = NULL;
        m_dh = NULL;
    }
        
    if (mLib_handle) {
        dlclose(mLib_handle);
        mLib_handle = NULL;
    }
    if (m_input) {
        stream_input_dispose(m_input);
    }
    //actal_ion_close();
    stream_buf_destroy();
    mQueue.clear();
}

size_t ActVideoExtractor::countTracks()
{
    EXTRACT_LOGV("countTracks(%d)", mTracks.size());
    return mTracks.size();
}

sp<MediaSource> ActVideoExtractor::getTrack(size_t index)
{
    if (index >= mTracks.size()) {
        ALOGE("getTrack() error =>  (index >= mTracks.size())");
        return NULL;
    }
    return new ActVideoSource(this, index);
}

sp<MetaData> ActVideoExtractor::getTrackMetaData(
        size_t index, uint32_t flags)
{
    if (index >= mTracks.size()) {
        ALOGE("getTrackMetaData() error => (index >= mTracks.size())");
        return NULL;
    }

    return mTracks.itemAt(index).mMeta;
}

void log_track_param(track_param_t* tp)
{
    AUDIOTRACK_LOGD("log_track_param:");
    AUDIOTRACK_LOGD("a_pos: 0x%08x", tp->a_pos);
    AUDIOTRACK_LOGD("a_pts: %d", tp->a_pts);
    AUDIOTRACK_LOGD("v_pos: 0x%08x", tp->v_pos);
    AUDIOTRACK_LOGD("v_pts: %d", tp->v_pts);
    AUDIOTRACK_LOGD("cut_t: %d", tp->cur_time);
    AUDIOTRACK_LOGD("index: %d", tp->track_num);
    AUDIOTRACK_LOGD("out_basetime: %d", tp->out_basetime);
}

uint32_t ActVideoExtractor::getFirstAudioTime()
{
    EXTRACT_LOGV("getFirstAudioTime: m_ts.found_audio_time: %d", m_ts.found_audio_time);
    return m_ts.found_audio_time;
}

status_t ActVideoExtractor::seek(int64_t seekTimeUs)
{
    Mutex::Autolock autoLock(mLock);
    EXTRACT_LOGT("seek: seekTimeUs(%lld)", seekTimeUs);
    CHECK(m_dp != NULL);
    CHECK(m_dh != NULL);

#ifdef MMM_ENABLE_SUBTITLE 
    {
        actal_memset(subtitle_data,0,MMM_SUBTITLE_DATA_LEN*MMM_SUBTITLE_NUM);
        subtitle_buf_No_0 = 0;
        subtitle_buf_No_1 = 0;
        subtitle_get_flag = 0;
        pkt_prv_ts = -1; 
        packet_header_tmp = NULL;
        packet_header_org = NULL;
        addr_oft = 0;   
        reserved_count = 0;
        put_subtitle_packet_flag = 0;
    }
#endif

    m_ts.seek_time = (unsigned int)((seekTimeUs + 500)/1000);
    m_ts.cur_time = 0;
    m_ts.found_audio_time = m_ts.found_video_time = m_ts.seek_time;

    m_pstatus = (plugin_err_no_t)m_dp->seek(m_dh, &m_ts);
    if (m_pstatus == PLUGIN_RETURN_NORMAL) {
        EXTRACT_LOGT("seek: seekTimeUs: %lld m_ts.found_audio_time: %d m_ts.found_video_time: %d m_ts.seek_time: %d", 
            seekTimeUs, m_ts.found_audio_time, m_ts.found_video_time, m_ts.seek_time);
        stream_buf_reset();
        mVpSpareData=NULL;
        mApSpareData=NULL;
        EXTRACT_LOGT("exit seek");
        return OK;
    } else {
        ALOGW("seek: call m_dp->seek() meet %s", m_pstatus==PLUGIN_RETURN_FILE_END ? "end of stream" : "error");
        if (m_pstatus == PLUGIN_RETURN_FILE_END) {
            return ERROR_END_OF_STREAM;
        }
        return !OK;
    }
}

status_t ActVideoExtractor::setAudioTrack(int index, int64_t cur_playing_time)
{
    Mutex::Autolock autoLock(mLock);

    ALOGD("%s:  total %d audio track, from %d to %d, current time: %d us)", __FUNCTION__, \
        m_mi.audio_num, m_ti_playing.a, index, (int32_t)cur_playing_time);

    if (index < 0 || (uint32_t)index > m_mi.audio_num ||
        m_ti_playing.a == (size_t)-1) {
        ALOGE("parameter err. index(%d)", index);
        return INVALID_OPERATION;
    }

    int pi = m_ti_playing.a /*- 1*/; /* !!! */
    if (m_mi.audio_num <= 1 || (size_t)pi == (size_t)index) {
        return INVALID_OPERATION;
    }

    sp<MetaData> meta = getTrackMetaData(index/*+1*/,0);
    const char *_mime;
    CHECK(meta->findCString(kKeyMIMEType, &_mime));
    String8 mime = String8(_mime);

    if (!(strncasecmp(mime.string(), "audio/unsupport", sizeof("audio/unsupport")))) {
        ALOGW("The current track format(%s) not support!!!", mime.string());
        return INVALID_OPERATION;
    }

    CHECK(m_dp != NULL);
    CHECK(m_dh != NULL);

    track_param_t tp;
    packet_header_t *ph;

    ph = (packet_header_t*)m_ao_buf.data;
    if (ph != NULL) {
        tp.a_pos = ph->packet_offset;
        tp.a_pts = ph->packet_ts;
    }

    ph = (packet_header_t*)m_vo_buf.data;
    if (ph != NULL) {
        tp.v_pos = ph->packet_offset;
        tp.v_pts = ph->packet_ts;
    }

    tp.cur_time = (int32_t)(cur_playing_time/1000);
    tp.out_basetime = 0;
    tp.track_num = index-1;

    log_track_param(&tp);

    if (0 != m_dp->ex_ops(m_dh, SET_TRACK, (uint32_t)&tp)) {
        ALOGE("setAudioTrack()->ex_ops SET_TRACK error!");
        return UNKNOWN_ERROR;
    }

    if ((tp.cur_time - tp.out_basetime) < 0) {
        ALOGE("ex_ops SET_TRACK error, cur_time:%d, out_baseTime:%d!", tp.cur_time, tp.out_basetime);
        return UNKNOWN_ERROR;
    }

    log_track_param(&tp);
    audio_stream_buf_reset();

    //stream_buf_reset();
    m_ti_playing.a = index /*+ 1*/;
    sp < MetaData > m = mTracks.itemAt(m_ti_playing.a).mMeta;
    m->setInt64(kKeyDriftTime, (((uint64_t)tp.cur_time)<<32) | ((uint64_t)(tp.cur_time - tp.out_basetime)));

    mIsSetTrack=true;
    AUDIOTRACK_LOGD("Set m_ti_playing.a to %d, audioOffset: %d ms", m_ti_playing.a, tp.cur_time - tp.out_basetime);

    return OK;
}

const char* ActVideoExtractor::setSubTrack(int index, int64_t cur_playing_time)
{
    Mutex::Autolock autoLock(mLock);
    SUB_LOGD("m_ti_playing.s(%d), m_mi.sub_num(%d), index(%d)", \
        m_ti_playing.a, m_mi.sub_num, index);

    if ((index < 0) || ((uint32_t)index >= m_mi.sub_num) ||
        (m_ti_playing.s == (size_t)-1)) {
        ALOGE("setSubTrack()->parameter err. index(%d)", index);
        return NULL;
    }

    int pi = m_ti_playing.s;
    if ((m_mi.sub_num <= 0) || (size_t)pi == (size_t)index)
        return (const char*)m_mi.parser_subtitle[index].buf;

    CHECK(m_dp != NULL);
    CHECK(m_dh != NULL);

    track_param_t tp;
    packet_header_t *ph = NULL;
    memset(&tp, 0, sizeof(track_param_t));
    ph = (packet_header_t*)m_ao_buf.data;
    if (ph!=NULL) {
        tp.a_pos = ph->packet_offset;
        tp.a_pts = ph->packet_ts;
    }
    ph = (packet_header_t*)m_vo_buf.data;
    if (ph!=NULL) {
        tp.v_pos = ph->packet_offset;
        tp.v_pts = ph->packet_ts;
    }
    tp.cur_time = (int32_t)(cur_playing_time/1000);
    tp.out_basetime = 0;
    tp.track_num = index;

    log_track_param(&tp);

    if (0 != m_dp->ex_ops(m_dh, SET_SUBPIC, (uint32_t)&tp)) {
        ALOGE("setSubTrack()->ex_ops SET_SUBPIC error!");
        return NULL;
    }
    
#ifdef MMM_ENABLE_SUBTITLE
    need_subtitle = 1;
#endif 
    m_ti_playing.s = index;

    return (const char*)m_mi.parser_subtitle[index].buf;
}

#ifdef MMM_ENABLE_SUBTITLE  
int64_t ActVideoExtractor::getsubtitle()
{
    if (m_mi.sub_num < 1) {
        ALOGW("no subtitle");
        return -1;
    }
    Mutex::Autolock autoLock(mLock);
    packet_header_t *packet_header =(packet_header_t *)subtitle_data[subtitle_buf_No_1];
    if (subtitle_get_flag == 0 && packet_header->header_type != 0&&packet_header->reserved2 >= 1) {
        SUB_LOGD("return packet_ts %d\n",packet_header->packet_ts);
        SUB_LOGD("getsubtitle: set subtitle_get_flag = 1"); 
        subtitle_get_flag = 1;
        return packet_header->packet_ts*1000;
    } else {
        //SUB_LOGD("subtitle_get_flag %d  packet_header->header_type %d",subtitle_get_flag,packet_header->header_type);
        return -1;
    }
}

int ActVideoExtractor::putsubtitle()
{
    Mutex::Autolock autoLock(mLock);
    packet_header_t *packet_header =(packet_header_t *)subtitle_data[subtitle_buf_No_1];
    av_buf_t sub_buf;
    int sub_buf_len,i,sub_buf_num,sub_buf_oft;
    unsigned char *ph = subtitle_data[subtitle_buf_No_1];
    SUB_LOGD("putsubtitle: reserved2 = %d, subtitle_buf_No_1 = %d",packet_header->reserved2,subtitle_buf_No_1);
    sub_buf_len = 0; 
    sub_buf_num = packet_header->reserved2;  
    if (packet_header->reserved2 >= 1) {
            //actal_memcpy((unsigned char *)args,subtitle_data[subtitle_buf_No_1],MMM_SUBTITLE_DATA_LEN);
            sub_buf.data = subtitle_data[subtitle_buf_No_1];
            sub_buf.data_len = MMM_SUBTITLE_DATA_LEN;
            SUB_LOGD("putsubtitle: set subtitle_get_flag = 0"); 
            subtitle_get_flag = 0;
            subtitle_buf_No_1++;
            if (subtitle_buf_No_1 == MMM_SUBTITLE_NUM) {
                subtitle_buf_No_1 = 0;
            }
            //packet_header->reserved2 = 0;
            for (i = 0;i < sub_buf_num;i++) {
                //SUB_LOGD("0packet_header->block_len %d",packet_header->block_len);
                sub_buf_oft = (packet_header->block_len + sizeof(packet_header_t) + 3)&(~3);
                sub_buf_len += sub_buf_oft;
                SUB_LOGD("putsubtitle: sub_buf_len = %d, sub_buf_oft = %d",sub_buf_len,sub_buf_oft);
                ph += sub_buf_oft;
                packet_header = (packet_header_t *)ph;                  
            }      
            notifyEvent(EXTRACTOR_SUBMIT_SUB_BUF, (int)(&sub_buf), sub_buf_len);    
            return 0;
    } else {
        ALOGW("putsubtitle return -1");
        return -1;
    }
}

int ActVideoExtractor::waitsubtitle()
{
    Mutex::Autolock autoLock(mLock);    
    SUB_LOGD("waitsubtitle: set subtitle_get_flag = 0");    
    subtitle_get_flag = 0;      
    return 0;    
}

int ActVideoExtractor::dropsubtitle()
{
    Mutex::Autolock autoLock(mLock);
    packet_header_t *packet_header =(packet_header_t *)subtitle_data[subtitle_buf_No_1];   
    SUB_LOGD("dropsubtitle: reserved2 = %d, subtitle_buf_No_1 = %d",packet_header->reserved2,subtitle_buf_No_1);   
    if (packet_header->reserved2 >= 1) {     
        SUB_LOGD("dropsubtitle: set subtitle_get_flag = 0");    
        subtitle_get_flag = 0;
        subtitle_buf_No_1++;
        if (subtitle_buf_No_1 == MMM_SUBTITLE_NUM) {
            subtitle_buf_No_1 = 0;
        }       
        return 0;
    } else {
        ALOGW("dropsubtitle return -1");
        return -1;
    }   
}

#endif

sp<MetaData> ActVideoExtractor::getMetaData()
{
    sp<MetaData> meta = new MetaData;

    char tmp[32] = {'v', 'i', 'd', 'e', 'o', '/', '\0'};
    char location[18] ={'+','0','0','.','0','0','0','0','-', '1','8','0','.','0','0','0','0','\0'};
    if (m_ext && strlen(m_ext) + strlen(tmp) <= sizeof(tmp)) {
        strcat(tmp, m_ext);
        EXTRACT_LOGV("getMetaData kKeyMIMEType(%s)", tmp);
    } else {
        ALOGE("getMetaData() => set kKeyMIMEType para. error");
    }

    meta->setCString(kKeyMIMEType, (const char *)tmp);
    meta->setPointer(kKeyActMediaInfo, (void*)(&m_mi));
    if (m_exist_location == true) {
        strcpy(location,(char*)(&(m_mi.v_len_array[3])));
        meta->setCString(kKeyLocation,(const char*)location);
    }

    return meta;
}

bool ActVideoExtractor::seekable()
{
    if (mExtractorFlags & (CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_SEEK)) {
        return true;
    } else {
        ALOGE("seekable: seek flag(0x%x) error!!!", mExtractorFlags);
        return false;
    }
}

uint32_t ActVideoExtractor::flags() const
{
    EXTRACT_LOGV("flags(0x%x)", mExtractorFlags);
    return mExtractorFlags;
}

inline int toupper(int c)
{
    c = ((c)>='a' && (c)<='z')? (c)-('a'-'A'):(c);
    return c;
}

char* strtoupper(char* dst, const char* src)
{
    const char* p = src;
    char* q = dst;
    if (!dst)
        return dst;

    do {
        *q++ = toupper(*p++);
    } while (*p != '\0');

    EXTRACT_LOGD("%s strtoupper is %s", src, dst);
    return dst;
}

status_t getNextNalUnit(
    const uint8_t **_data, size_t *_size,
    const uint8_t **nalStart, size_t *nalSize,
    bool startCodeFollows) {
    const uint8_t *data = *_data;
    size_t size = *_size;

    *nalStart = NULL;
    *nalSize = 0;

    if (size == 0) {
        return -EAGAIN;
    }

    // Skip any number of leading 0x00.

    size_t offset = 0;
    while (offset < size && data[offset] == 0x00) {
        ++offset;
    }

    if (offset == size) {
        return -EAGAIN;
    }

    // A valid startcode consists of at least two 0x00 bytes followed by 0x01.

    if (offset < 2 || data[offset] != 0x01) {
        return ERROR_MALFORMED;
    }

    ++offset;

    size_t startOffset = offset;

    for (;;) {
        while (offset < size && data[offset] != 0x01) {
            ++offset;
        }

        if (offset == size) {
            if (startCodeFollows) {
                offset = size + 2;
                break;
            }
            return -EAGAIN;
        }

        if (data[offset - 1] == 0x00 && data[offset - 2] == 0x00) {
            break;
        }

        ++offset;
    }

    size_t endOffset = offset - 2;
    while (endOffset > startOffset + 1 && data[endOffset - 1] == 0x00) {
        --endOffset;
    }

    *nalStart = &data[startOffset];
    *nalSize = endOffset - startOffset;

    if (offset + 2 < size) {
        *_data = &data[offset - 2];
        *_size = size - offset + 2;
    } else {
        *_data = NULL;
        *_size = 0;
    }

    return OK;
}


bool getAVCHeaderInfo(void *buf,  int32_t *Length)
{
    const uint8_t *nalStart = NULL;
    const uint8_t *seqStart = NULL;
    const uint8_t * data = NULL;
    size_t nalSize = 0;
    size_t size = 0;

    int32_t headerLen = 0;
    uint8_t getHeaderFlag = 0;
    int32_t seqLen = 0;

    data = (uint8_t *)buf;
    size = 1024; 

    ALOGI("getAVCHeaderInfo--: data: 0x%x size: %d", (void *)data, size);
    while (getNextNalUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
        unsigned nalType = nalStart[0] & 0x1f;

        if (nalType == 7) {
            headerLen += nalSize + 4;
            getHeaderFlag |= 1;
            ALOGI("getAVCHeaderInfo-0: nalType: %d nalSize: %d headerLen: %d getHeaderFlag: %d", nalType, nalSize, headerLen, getHeaderFlag);
        } else if (nalType == 8) {
            headerLen += nalSize + 4;
            getHeaderFlag |= 2; 
            ALOGI("getAVCHeaderInfo-1: nalType: %d nalSize: %d headerLen: %d getHeaderFlag: %d", nalType, nalSize, headerLen, getHeaderFlag);
        } else {
            ALOGI("getAVCHeaderInfo-2: nalType: %d nalSize: %d headerLen: %d getHeaderFlag: %d", nalType, nalSize, headerLen, getHeaderFlag);
        }
        
        if ((getHeaderFlag & 0x3) == 0x3) {
            seqLen = headerLen + ((int32_t)seqStart - (int32_t)buf);
            ALOGI("getAVCHeaderInfo-3: nalType: %d nalSize: %d headerLen: %d getHeaderFlag: %d seqStart: 0x%x buf: 0x%x", 
                    nalType, nalSize, headerLen, getHeaderFlag, (void *)seqStart, (void *)buf);
            break;
        } else if ((getHeaderFlag & 0x3) == 0x1) {
             seqStart = nalStart - 4;
        } else{
        }
        
    }

    if (0) {
        FILE *fp1=fopen("/data/juan/seq1.264", "a+");
        if (fp1 == NULL) {
            ALOGE("open /data/juan/seq1.264 failed");
        } else {
            fwrite(buf, 1, seqLen, fp1);
            fclose(fp1);
        }
    }

    ALOGI("getAVCHeaderInfo-4: headerLen: %d seqLen: %d getHeaderFlag: %d seqStart: 0x%x buf: 0x%x", 
            headerLen, seqLen, getHeaderFlag, (void *)seqStart, (void *)buf);
    *Length = seqLen;
    return true;
}

void ActVideoExtractor::addTracks()
{
    /* clear now playing track indexs*/
    m_ti_playing.s = m_ti_playing.a = m_ti_playing.v = (size_t)-1;

    if (!m_mi.index_flag) {
        mExtractorFlags |= CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_SEEK | CAN_PAUSE;
        EXTRACT_LOGD("addTracks: set index_flag(0x%x) mExtractorFlags(0x%x)", m_mi.index_flag, mExtractorFlags);
    }

    /* video */
    parser_video_t *pv = &m_mi.parser_video;
    sp<MetaData> meta = new MetaData;
    if (pv->buf) {
    #if 0 // For video_1_demo.mkv seek problem
        if (!strcasecmp(pv->extension, "h264")) {
            int32_t Length = 0;
            getAVCHeaderInfo(pv->buf, &Length);
            meta->setInt32(kKeyActVDHrdLen, Length);
            ALOGI("call getAVCHeaderInfo(pv->buf: 0x%x Length: %d)", pv->buf, Length);
        }
    #endif
        bool mNotNeedkKeyActVDPrv = (strncmp("mp4", m_ext, 3) == 0 && strncmp("mpeg", pv->extension, 4) == 0);
        if (!mNotNeedkKeyActVDPrv) {
            meta->setPointer(kKeyActVDPrv, pv->buf);
        }  

        EXTRACT_LOGD("addTracks: sets Actions' VCodecs' privdata ");
    }
    size_t kNumExtToMime = sizeof(kExtToMime) / sizeof(kExtToMime[0]);
    size_t i;
    if (!strcasecmp(pv->extension, "mpeg") && ((pv->width+15)&(~15))>720 && m_cookie != NULL){
        strcat(pv->extension,"_4k");
        ALOGD("mpeg2 interlaced \n");
    }

    for (i = 0; i < kNumExtToMime; ++i) {
        if (!strcasecmp(pv->extension, kExtToMime[i].ext)) {
            mEtm = &kExtToMime[i];
            break;
        }
    }

    if (!strncmp("mp4", m_ext, 3)) {
        if (m_mi.v_len_array[0] > 0) {
            meta->setInt32(kKeyRotation, m_mi.v_len_array[1]);
        } else {
            m_mi.v_len_array[1] = 0;
        }   
        if (m_mi.v_len_array[2] > 0) {
            m_exist_location = true;
        }
    }

    CHECK(mEtm->mime != NULL);
    if (!strcasecmp(mEtm->mime, "video/unsupport")) {
        mNotPlayVideo = true;
        ALOGW(" Not support video format(%s)",pv->extension);       
    }
    meta->setCString(kKeyMIMEType, mEtm->mime);  
    ALOGD("kKeyWidth/kKeyHeight set with no alignment");
    meta->setInt32(kKeyWidth, /*(pv->width + 15)&(~15)*/pv->width);
    meta->setInt32(kKeyHeight, /*(pv->height + 15)&(~15)*/pv->height);
    meta->setInt32(kKeyBitRate, (int32_t)m_mi.parser_video.video_bitrate);
    meta->setInt64(kKeyActFrmRate, (int64_t)m_mi.parser_video.frame_rate);
    mVDuration = mADuration = ((int64_t)(m_mi.total_time))*1000;
    meta->setInt64(kKeyDuration, mVDuration);
    CHECK(m_pkt_maxs.v);
    ALOGD("addTracks: video format(%s) kKeyMIMEType: %s kKeyMaxInputSize: %d bytes", pv->extension,  mEtm->mime, m_pkt_maxs.v);
    meta->setInt32(kKeyMaxInputSize, (int32_t)m_pkt_maxs.v);
    
    mTracks.push();
    TrackInfo *trackInfo = &mTracks.editItemAt(mTracks.size() - 1);
    trackInfo->mMeta = meta;
    m_ti_playing.v = 0; /* video track index */

    /* audio */
    for (uint32_t i = 0; i < m_mi.audio_num; i++) {
        parser_audio_t *pa = &m_mi.parser_audio[i];
        sp<MetaData> meta = new MetaData;
        const char* ext = NULL;
        char mimetmp[18] = {'a', 'u', 'd', 'i', 'o', '/', '\0'};
        size_t j;
        kNumExtToMime = sizeof(kAExtToMime) / sizeof(kAExtToMime[0]);
        for (j= 0; j < kNumExtToMime; ++j) {
            if (!strcasecmp(pa->extension, kAExtToMime[j].ext)) {
                mAEtm = &kAExtToMime[j];
                break;
            }
        }       
        CHECK(mAEtm->mime != NULL);
        if (!strcasecmp(mAEtm->mime, "unsupport")) {    
            mUnsupportAudioTrackNum++;          
            ALOGW(" Not support audio format(%s)",pa->extension);           
        }
        memcpy(&mimetmp[6], mAEtm->mime, mAEtm->len);
        ALOGD("addTracks: pa->extension: %s mimetmp: %s    mAEtm->mime: %s", pa->extension, mimetmp, mAEtm->mime);
        meta->setCString(kKeyMIMEType, mimetmp);  
        meta->setInt32(kKeySampleRate, pa->sample_rate);
        meta->setInt32(kKeyChannelCount, pa->channels);
        meta->setInt64(kKeyDuration, mADuration);
        meta->setInt32(kKeyBitRate, (int32_t)pa->audio_bitrate);
        meta->setInt32(kKeyMaxInputSize, (int32_t)m_pkt_maxs.a);
        
        if (pa->buf && strcasecmp(mAEtm->mime, "unsupport")) {
            meta->setPointer(kKeyESDS, (void*)(((uint8_t*)pa->buf) + SIZE_PHT));
            ALOGE("initbuf:%x\n",((uint8_t*)pa->buf) + SIZE_PHT);
        }

        mTracks.push();
        TrackInfo *trackInfo = &mTracks.editItemAt(mTracks.size() - 1);
        trackInfo->mMeta = meta;
        if (i == 0) {
            m_ti_playing.a = 1; /* audio track index */
        }
    }
 
    if (mUnsupportAudioTrackNum == m_mi.audio_num) {
        ALOGW("Not play audio!");       
        mNotPlayAudio = true;
    }
    if (m_ti_playing.a == (size_t)-1) {
        //ALOGD("-------------- %s, m_ti_playing.a(%d) ", __FUNCTION__, m_ti_playing.a);
        mAlwaysDropAudio = 1;
    }
    /* subtitle */
    for (uint32_t i=0; i<m_mi.sub_num; i++) {
        parser_subtitle_t *ps = &m_mi.parser_subtitle[i];
        sp<MetaData> meta = new MetaData;
        char mimetmp[24] = {'s', 'u', 'b', 't', 'i', 't', 'l', 'e', '/', '\0'};
        strtoupper(&mimetmp[strlen(mimetmp)], (const char*)ps->extension);
        EXTRACT_LOGD("addTracks: mime(%d): %s", i, mimetmp);
        meta->setCString(kKeyMIMEType, (const char*)mimetmp);
        mTracks.push();
        TrackInfo *trackInfo = &mTracks.editItemAt(mTracks.size() - 1);
        trackInfo->mMeta = meta;
        if (i == 0) {
            m_ti_playing.s = 0;
        }
    }
    EXTRACT_LOGV("exit addTracks()  OK");
}

}  // namespace android