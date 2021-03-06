/*
 * Copyright (C) 2009 The Android Open Source Project
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
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

#ifndef MEDIA_DEFS_H_

#define MEDIA_DEFS_H_

namespace android {

extern const char *MEDIA_MIMETYPE_IMAGE_JPEG;

extern const char *MEDIA_MIMETYPE_VIDEO_VPX;
extern const char *MEDIA_MIMETYPE_VIDEO_AVC;
extern const char *MEDIA_MIMETYPE_VIDEO_MPEG4;
extern const char *MEDIA_MIMETYPE_VIDEO_H263;
extern const char *MEDIA_MIMETYPE_VIDEO_MPEG2;
extern const char *MEDIA_MIMETYPE_VIDEO_RAW;
extern const char *MEDIA_MIMETYPE_VIDEO_DIV3;
extern const char *MEDIA_MIMETYPE_VIDEO_FLV;
extern const char *MEDIA_MIMETYPE_VIDEO_WMV8;

extern const char *MEDIA_MIMETYPE_AUDIO_AMR_NB;
extern const char *MEDIA_MIMETYPE_AUDIO_AMR_WB;
extern const char *MEDIA_MIMETYPE_AUDIO_MPEG;           // layer III
extern const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_I;
extern const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II;
extern const char *MEDIA_MIMETYPE_AUDIO_AAC;
extern const char *MEDIA_MIMETYPE_AUDIO_QCELP;
extern const char *MEDIA_MIMETYPE_AUDIO_VORBIS;
extern const char *MEDIA_MIMETYPE_AUDIO_G711_ALAW;
extern const char *MEDIA_MIMETYPE_AUDIO_G711_MLAW;
extern const char *MEDIA_MIMETYPE_AUDIO_RAW;
extern const char *MEDIA_MIMETYPE_AUDIO_FLAC;
extern const char *MEDIA_MIMETYPE_AUDIO_AAC_ADTS;
extern const char *MEDIA_MIMETYPE_AUDIO_DTS;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_AAC;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_MP3;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_PCM;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_OGG;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_DTS;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_AC3;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_APE;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_FLAC;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_MPC;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_AIFF;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_AMR;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_SAMPLE;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_ALAC;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_WMASTD;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_WMALSL;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_WMAPRO;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_COOK;
extern const char *MEDIA_MIMETYPE_AUDIO_ACT_AWB;

extern const char *MEDIA_MIMETYPE_CONTAINER_MPEG4;
extern const char *MEDIA_MIMETYPE_CONTAINER_WAV;
extern const char *MEDIA_MIMETYPE_CONTAINER_OGG;
extern const char *MEDIA_MIMETYPE_CONTAINER_MATROSKA;
extern const char *MEDIA_MIMETYPE_CONTAINER_MPEG2TS;
extern const char *MEDIA_MIMETYPE_CONTAINER_AVI;
extern const char *MEDIA_MIMETYPE_CONTAINER_MPEG2PS;

extern const char *MEDIA_MIMETYPE_CONTAINER_WVM;

extern const char *MEDIA_MIMETYPE_TEXT_3GPP;
extern const char *MEDIA_MIMETYPE_TEXT_SUBRIP;

#ifdef QCOM_HARDWARE
extern const char *MEDIA_MIMETYPE_AUDIO_AC3;
extern const char *MEDIA_MIMETYPE_AUDIO_AMR_WB_PLUS;
extern const char *MEDIA_MIMETYPE_AUDIO_DTS;
extern const char *MEDIA_MIMETYPE_AUDIO_DTS_LBR;
extern const char *MEDIA_MIMETYPE_AUDIO_EAC3;
extern const char *MEDIA_MIMETYPE_AUDIO_EVRC;
extern const char *MEDIA_MIMETYPE_AUDIO_WMA;

extern const char *MEDIA_MIMETYPE_CONTAINER_3G2;
extern const char *MEDIA_MIMETYPE_CONTAINER_AAC;
extern const char *MEDIA_MIMETYPE_CONTAINER_ASF;
extern const char *MEDIA_MIMETYPE_CONTAINER_MPEG2;
extern const char *MEDIA_MIMETYPE_CONTAINER_QCP;

extern const char *MEDIA_MIMETYPE_VIDEO_DIVX;
extern const char *MEDIA_MIMETYPE_VIDEO_DIVX311;
extern const char *MEDIA_MIMETYPE_VIDEO_DIVX4;
extern const char *MEDIA_MIMETYPE_VIDEO_WMV;
#endif

}  // namespace android

#endif  // MEDIA_DEFS_H_
