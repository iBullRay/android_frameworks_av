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

#ifndef ACTVIDEO_EXTRACTOR_H_

#define ACTVIDEO_EXTRACTOR_H_

#include <media/stagefright/MediaExtractor.h>
#include <utils/Vector.h>
#include <utils/List.h>
#include <media/stagefright/MediaBuffer.h>
#include <actal_posix_dev.h>
#include <ALdec_plugin.h>

#define TURN_ON_ADD_VIDEO_DECODE_FLAG 1 // Mainly include vp6/vp8/rvg2
#define TURN_ON_STREAM_BUFFER_MANAGE_FLAG 0 // can print stream_buffer manage log

namespace android {

//struct AMessage;
//class String8;

//struct ActVideoSource;
//
//typedef packet_header_t _key;
//
//List<short> m_strfifo; /* stream fifo */

enum EVENT_T {
    EXTRACTOR_SUBMIT_SUB_BUF = 0,
};

typedef struct {
    size_t a;
    size_t v;
    size_t s;
} track_index_t;

typedef struct {
    size_t a;
    size_t v;
    size_t s;
} packet_max_size_t;

typedef struct {
    uint8_t* a;
    uint8_t* b;
} region_t;

typedef struct {
    uint8_t* start;
    uint8_t* start_phyaddr;
    size_t size;
    size_t real_size;
    region_t empty;
} stream_buf_t;

typedef enum {
    CODEC_CODA,
    CODEC_ACTIONS,
    CODEC_SOFTWARE,
    CODEC_UNKNOWN
} CODEC_TYPE;

typedef struct ExtToMime {
    const char *ext;
    const char *mime;
    CODEC_TYPE ct;
} ExtToMime_t;

typedef struct aExtToMime{
    const char * ext;
    const char * mime;
    uint32_t len;
}aExtToMime_t;

#define MMM_SUBTITLE_NUM 30
#define MMM_SUBTITLE_DATA_LEN 15360

struct ActVideoExtractor : public MediaExtractor {
    ActVideoExtractor(const sp<DataSource> &source, const char* mime, void* cookie = NULL);

    virtual size_t countTracks();

    virtual sp<MediaSource> getTrack(size_t index);

    virtual sp<MetaData> getTrackMetaData(
            size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();
    virtual status_t setAudioTrack(int index, int64_t cur_playing_time);
    virtual const char* setSubTrack(int index, int64_t cur_playing_time);

    #ifdef MMM_ENABLE_SUBTITLE  
        virtual int64_t getsubtitle();
        virtual int putsubtitle();
        virtual int waitsubtitle();
        virtual int dropsubtitle();
    #endif

    virtual uint32_t flags() const;

protected:
    virtual ~ActVideoExtractor();

    private:
    friend struct ActVideoSource;

    struct TrackInfo {
        unsigned long mTrackNum;
        sp<MetaData> mMeta;
    };
    Vector<TrackInfo> mTracks;

    ActVideoExtractor(const ActVideoExtractor &);
    ActVideoExtractor &operator=(const ActVideoExtractor &);

    /* demuxer handle, demuxer function */
    void* m_dh;
    demux_plugin_t *m_dp;
    media_info_t m_mi;
    void* mLib_handle;

    /* main functions */
    bool test();
    bool input_mi_init();
    bool open_plugin(const char *ext);
    void addTracks();
    status_t getPacket(av_buf_t* av_buf, packet_type_t t);
    bool seekable();
    status_t seek(int64_t seekTimeUs);
    uint32_t getFirstAudioTime();
    packet_type_t matchPacketType(size_t track_idx);

    /* stream input */
    stream_input_t *m_input;
    char m_ext[24];
    bool m_exist_location;

    /* now playing track index */
    track_index_t m_ti_playing;

    /* for seek */
    time_stuct_t m_ts;
    /* Normally, demuxer's packet size should NOT excced this size */
    packet_max_size_t m_pkt_maxs;
    /* stream buffer assosiated */
    stream_buf_t m_ap;
    stream_buf_t m_vp;
    bool mIsSetTrack;
    //stream_buf_t m_sp;g
    bool stream_buf_destroy();
    void stream_buf_reset();
    bool stream_buf_init();
    bool stream_buf_prepare(av_buf_t* ao_buf, av_buf_t* vo_buf);
    bool stream_buf_empty_area_update(av_buf_t *av_buf, bool enlarge);
    bool stream_buf_assign(packet_type_t type, av_buf_t* av_buf);
    bool stream_buf_check_overflow(av_buf_t *av_buf);
    void stream_buf_status_dump(stream_buf_t* t);
    void stream_buf_quelst(packet_type_t type);
    void audio_stream_buf_reset();
    mutable Mutex mLock;
    Mutex mMiscStateLock;

    /* stream fifo assosiated */
    List<av_buf_t> mQueue;
    stream_buf_t m_vp_spare;
    stream_buf_t m_ap_spare;
    bool mUsingSpare;
    bool mVpUsingSpare;
    bool mApUsingSpare;
    uint8_t *mVpSpareData;
    uint8_t *mApSpareData;

    av_buf_t m_ao_buf;
    av_buf_t m_vo_buf;
    void avbufItemAdd(av_buf_t* av_buf);
    bool avbufItemPick(packet_type_t t, av_buf_t* av_buf);
    bool avbufItemCheck();

    /* notify event to Owner */
    void* m_cookie;
    status_t notifyEvent(EVENT_T msg, int ext1, int ext2);

    /* demuxer plugin status */
    plugin_err_no_t m_pstatus;
    bool m_fstatus;
    int32_t mCounter;

    uint32_t mExtractorFlags;
    const ExtToMime_t *mEtm;
    const aExtToMime_t *mAEtm;
    /* when video seek failed, set this flag == 1, and audio seek won't work. */
    int mSeekFailed;
    int mAlwaysDropAudio;
    int mVideoAlreadySeek;

    #ifdef MMM_ENABLE_SUBTITLE  
    unsigned char subtitle_data[MMM_SUBTITLE_NUM][MMM_SUBTITLE_DATA_LEN];   
    int subtitle_buf_No_0;
    int subtitle_buf_No_1;
    int subtitle_get_flag;
    int pkt_prv_ts;
    packet_header_t *packet_header_tmp;
    packet_header_t *packet_header_org;
    int addr_oft;   
    int reserved_count;
    int put_subtitle_packet_flag;
    int need_subtitle;
    #endif
    int64_t mADuration;
    int64_t mVDuration;
    int64_t mVLastPktTime;
    int64_t mALastPktTime;

    bool mNotPlayVideo;
    bool mNotPlayAudio;
    uint32_t mUnsupportAudioTrackNum;
    bool mUsingPhyContBuffer;

};

}  // namespace android

#endif  // ACTVIDEO_EXTRACTOR_H_