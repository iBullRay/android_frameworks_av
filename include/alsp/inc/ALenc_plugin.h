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

#ifndef __ALENC_PLUGIN_H__
#define __ALENC_PLUGIN_H__

#include "./common/al_libc.h"
#include "./common/stream_input.h"

/* disk manager seek */
#define DSEEK_SET 0x01
#define DSEEK_END 0x02
#define DSEEK_CUR 0x03

#ifndef NULL
#define NULL 0
#endif

/* used by muxer packet flag */
#define IS_AUDIO 0x01 //contains only audio
#define IS_VIDEO 0x02 //contains only video   
#define IS_AV 0x03 //contains audio and video

/* type */
#define PLUGIN_MUX 0x05 // plugin for muxer
#define PLUGIN_VIDEO_ENCODER 0x07 // plugin for video encoder
#define PLUGIN_IMAGE_ENCODER 0x08 // plugin for img encoder

typedef struct {     
    unsigned int formate;
    unsigned int bpp;
    unsigned int width;
    unsigned int height;
    unsigned char *buf;
    unsigned int len;
    unsigned int bk_buf;
    unsigned int bk_width;
    unsigned int bk_height;
}image_info_t;

typedef struct {
    unsigned int photo_data_flag;
    unsigned char *photo_data;
    unsigned int pd_width;
    unsigned int pd_height;
    unsigned char *exif_info;
    unsigned int scale_mode;
    unsigned int func_mode;
    unsigned int dst_width;
    unsigned int dst_height;
    unsigned int fr;
    unsigned int br;
}camera_init_t;

#define SET_FACE_MASK_ON 0xb100
#if 0
typedef struct {  
    ALFace_appout_t *face_app;
    image_info_t img;
    unsigned int bk_info;
}ALFace_info_t;
#endif

typedef struct camera_plugin_s {
    int (*init)(void *plugin, camera_init_t *param);
    int (*encode_img)(void *plugin, image_info_t *img);
    int (*dispose)(void *plugin);
    int (*exop)(void *plugin,int cmd,unsigned int args);
}camera_plugin_t;

typedef  struct {
    int enable_display;
    int width;
    int height;
} display_resolution_t;

typedef struct {
    char *audio_fmt;
    unsigned int bpp;
    unsigned int bitrate;
    unsigned int sample_rate;
    unsigned int channels;
    unsigned int encode_mode;
}ae_param_t;

typedef struct {
    int width;
    int height;
    int src_width;
    int src_height;
    int fincr;
    int fbase;
    int framerate; 
    int max_key_interval;
    int quanty;       
    int frame_drop_ratio;
    int max_bframes;
    int video_bitrate;
    int bQOffset;
    int bQRatio;
    int cardtype;
    int reserved[9];
}ve_param_t;

typedef struct {
    char file_fmt[12];
    unsigned int streamer_type; 
    ae_param_t audio_param;
    ve_param_t video_param;
    char audio_fmt[12];
    char video_fmt[12];
    unsigned char *tbl_buf;
    unsigned int tbl_buf_len;
}ave_param_t;

typedef struct {
    unsigned char *data;
    int  data_len;
    int  is_key_frame;
    int  encode_type;
}encode_frame_t;

typedef struct {
    int media_type;
    char *data;
    int data_len;
    int is_key_frame;
    int  time_stamp;
    int reserved[9];
}av_chunk_t;

typedef struct {
    ave_param_t *ave_param;
    stream_input_t *input;
    unsigned int reserved[2];
}mux_input_attr_t;  

#define GET_PACKET_TYPE 0xc001                                 
#define GET_MUXER_STATUS 0xc002   
#define RESET_MUXER 0xc003

typedef enum {
    EITHER_OK,
    AUDIO_PACKET,
    VIDEO_PACKET
}packet_type_t;

typedef struct {
    unsigned int total_frames;
    unsigned int video_frames;
    unsigned int audio_frames;
    unsigned int movi_length_L;
    unsigned int movi_length_H;
    unsigned int reserved[5];
}mux_output_attr_t;

typedef struct {
    char *data;
    int data_len;
    int is_key_frame;
    int encode_type;
}av_frame_t;

#define GET_ENCODER_ATTR 0xb001  
#define GET_ENCODER_STATUS 0xb002  
#define SET_DISPLAY_PARAM 0xb005    

typedef struct {
    unsigned int frame_type;
    unsigned int frame_size;
    unsigned int cur_quality;
    unsigned int cur_interval;
    unsigned int cur_suv_status;
    unsigned int display_buf;
    unsigned int inter_param;
    unsigned short out_width;
    unsigned short out_height;
    unsigned short out_format;
}videoenc_output_attr_t;            

#define SET_MOTION_PARAM 0xb003   
#define SET_ZOOM_PARAM 0xb004   
#define SET_SOURCE_WINDOWN 0xb008
#define SET_FRM_INTVAL 0xb009

typedef struct {
    unsigned int cur_quality;
    unsigned int cur_interval;
    int sur_field_mode;
    unsigned short md_throld0;
    unsigned short md_throld1;
    unsigned short md_throld2;
    unsigned short md_throld3;
    unsigned int reserved[3];
}videoenc_input_attr_t;

typedef struct {
    int enable_zoom;
    int fr;
    int br;
}encoder_zoom_t;

typedef struct {
    int width;
    int height;
}encoder_srcwin_t;

typedef struct {
    char type;
    char *extension;
    void *(*open)(void *plugio);
    int (*get_file_info)(stream_input_t *input,void *file_info);
}plugin_info_t;

typedef struct video_encoder_s {
    int (*init)(struct video_encoder_s *plugin,ve_param_t *param);
    int (*encode_data)(struct video_encoder_s *plugin,av_frame_t *src_frame,av_frame_t *dest_frame);
    int (*set_attribute)(struct video_encoder_s *plugin,int attrib_id,void *param);
    int (*get_attribute)(struct video_encoder_s *plugin,int attrib_id,void *param);
    int (*get_err)(struct video_encoder_s *plugin);
    int (*dispose)(struct video_encoder_s *plugin);
}video_encoder_t;

typedef struct mux_plugin_s {
    int (*init)(struct mux_plugin_s *plugin,ave_param_t *param);
    int (*write_header)(struct mux_plugin_s *plugin);
    int (*write_chunk)(struct mux_plugin_s *plugin,av_chunk_t *chunk);
    int (*set_attribute)(struct mux_plugin_s *plugin,int attrib_id,void *param);
    int (*get_attribute)(struct mux_plugin_s *plugin,int attrib_id,void *param);
    int (*get_error)(struct mux_plugin_s *plugin);
    int (*dispose)(struct mux_plugin_s *plugin);
}mux_plugin_t;

#endif
