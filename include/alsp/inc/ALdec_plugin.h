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

#ifndef ALDEC_PLUGIN_H
#define ALDEC_PLUGIN_H

#ifdef WIN32
#include "al_libc_dll.h"
#endif
#include "./common/stream_input.h"
#include "./common/extdef.h"
#include "./common/buf_header.h"

#define _ISPRAM_CODE_
#define _ISPRAM_SECTION_NAME_

#ifdef __cplusplus
extern "C" {
#endif

#define YUV420_SEMI_PLANAR 0x00
#define YUV420_PLANAR 0x01

#define AUDIO_MAX_PKT_SIZE 204800
#define VIDEO_MAX_PKT_SIZE 2048000

typedef enum {
    PLUGIN_RETURN_ERR = -1,
    PLUGIN_RETURN_NORMAL = 0,
    PLUGIN_RETURN_FILE_END,
    PLUGIN_RETURN_NOT_SUPPORT,
    PLUGIN_RETURN_PKT_NOTEND,
    PLUGIN_RETURN_SEEK_ERR
}plugin_err_no_t;


typedef struct {
    unsigned char *data;
    unsigned int data_len;
}av_buf_t;

typedef enum {
    SET_SUBPIC,
    SET_TRACK,
    NORMAL_PLAY,
    DISCARD_FRAMES,
    NOTIFY_REF_FRAME_NUM,
    NOTIFY_FIFO_RESET,
    WAIT_FOR_HANTRO,
    SET_FOURCC,
    FLUSH_BUFFERS_REMAINING,
    SET_SEEK_OPTIONS = 0x100,
    NOTIFY_MEDIA_TYPE,
    RESET_PARSER,
    EX_RESERVED1,
    EX_RESERVED2
}plugin_ex_ops_cmd_t;

typedef struct {
    unsigned int a_pos; 
    unsigned int v_pos; 
    int a_pts;
    int v_pts;
    int cur_time;
    int subpic_num;
}subpic_param_t;

typedef struct {
    unsigned int a_pos; 
    unsigned int v_pos; 
    int a_pts;
    int v_pts;
    int cur_time;
    int track_num;  
    int out_basetime;
}track_param_t;

typedef enum {
    AVI = 0,
    MP4,
    MKV
} fileFormat_t;

typedef struct {
    unsigned int audio_offset;
    unsigned int audio_ts;
    unsigned int video_offset;
    unsigned int video_ts;
    unsigned int subpic_offset;
    unsigned int subpic_ts;
}switch_audio_subpic_t;

typedef enum {
    IS_AUDIO = 1,
    IS_VIDEO,
    IS_AV
}media_type_t;

typedef struct {
    char extension[8];  
    unsigned int sample_rate;
    unsigned int channels;
    void *buf;
    void *private_data;
    unsigned int audio_bitrate;
    unsigned int a_max_pkt_size;
} parser_audio_t;

typedef struct {
    char extension[8];
    unsigned int width;
    unsigned int height;
    unsigned int frame_rate;
    void *buf;
    unsigned int video_bitrate;
    unsigned int v_max_pkt_size;
}parser_video_t;

typedef struct {
    unsigned int drm_flag;
    char *license_info;
    char *special_info;
}parser_drm_t;

typedef struct {
    char extension[8];  
    void *buf;
    void *private_data;
} parser_subtitle_t;

typedef struct { 
    parser_audio_t parser_audio[16];
    parser_video_t parser_video;
    parser_drm_t parser_drm;
    parser_subtitle_t parser_subtitle[16];
    unsigned int sub_num;
    unsigned int audio_num;
    unsigned int media_type;
    unsigned int total_time;
    unsigned int first_audio_time;
    unsigned int index_flag;
    unsigned int a_len_array[32];
    unsigned int v_len_array[32];
}media_info_t;

typedef enum {
    PLUGIN_SUPPORTED,
    PLUGIN_NOT_SUPPORTED_FIELDMOD,
    PLUGIN_NOT_SUPPORTED_YUV444,
    PLUGIN_NOT_SUPPORTED_GMC,
    PLUGIN_NOT_SUPPORTED_RPR,
    PLUGIN_NOT_SUPPORTED_ADVANCED,
    PLUGIN_NOT_SUPPORTED_OTHER
}video_plugin_supported_t;

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int src_width;
    unsigned int src_height;
    unsigned int xpos;
    unsigned int ypos;
    unsigned int ref_num;
    unsigned int extra_frame_size;
    video_plugin_supported_t supported;
}video_codec_info_t;

typedef struct {
    void *(*get_wbuf)(struct fb_port_s *port,unsigned int buf_size);
    void *(*try_get_wbuf)(struct fb_port_s *port,unsigned int buf_size);
}fb_port_t;

typedef struct {
    unsigned int display_flag;
    unsigned int use_flag; 
    unsigned int time_stamp;
    unsigned int width;
    unsigned int height;
    unsigned int format;
    unsigned int reserved1;
    unsigned int reserved2;
}dec_buf_t;

typedef struct {
    dec_buf_t *vo_frame_info;
    unsigned char* vir_addr;
    unsigned char* phy_addr;
    unsigned int size;
}frame_buf_handle;

typedef struct {
    unsigned char *vir_addr;
    unsigned char *phy_addr;
    unsigned int data_len;
    void *reserved;
}stream_buf_handle;

typedef struct {
    char file_extension[8];
    void *(*open)(void *input,void *media_info);
    int (*parse_stream)(void *handle,av_buf_t *ao_buf,av_buf_t *vo_buf);
    int (*seek)(void *handle,void *time4seek);
    int (*dispose)(void *handle);
    int (*ex_ops)(void *handle,int cmd,unsigned int arg);
}demux_plugin_t;

typedef struct {
    char extension[8];
    void *(*open)(void *ap_param,void *init_param,void *fb_vo);
    int (*decode_data)(void *handle,stream_buf_handle *bitstream_buf);
    int (*dispose)(void *handle);
    int (*ex_ops)(void *handle,int cmd,unsigned int arg);
    int (*probe)(void *init_buf,stream_buf_handle *bitstream_buf,video_codec_info_t *info);
}videodec_plugin_t;

typedef struct {
    unsigned int seek_time;
    unsigned int cur_time;
    unsigned int found_audio_time;
    unsigned int found_video_time;
} time_stuct_t;

#ifdef __cplusplus
}
#endif
#endif
