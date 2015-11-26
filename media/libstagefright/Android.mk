LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/av/media/libstagefright/codecs/common/Config.mk

ifeq ($(BOARD_HTC_3D_SUPPORT),true)
   LOCAL_CFLAGS += -DHTC_3D_SUPPORT
endif

LOCAL_SRC_FILES:=                         \
        ACodec.cpp                        \
        AACExtractor.cpp                  \
        AACWriter.cpp                     \
        ActAudioExtractor.cpp             \
        ActDataSource.cpp                 \
        ActAudioDownMix.cpp               \
        ActAudioDecoder.cpp               \
        ActAudioEncoder.cpp               \
        ActAudioWriter.cpp                \
        ActVideoExtractor.cpp             \
        AMRExtractor.cpp                  \
        AMRWriter.cpp                     \
        AudioPlayer.cpp                   \
        AudioSource.cpp                   \
        AwesomePlayer.cpp                 \
        CameraSource.cpp                  \
        CameraSourceTimeLapse.cpp         \
        DataSource.cpp                    \
        DRMExtractor.cpp                  \
        ESDS.cpp                          \
        FileSource.cpp                    \
        FLACExtractor.cpp                 \
        FragmentedMP4Extractor.cpp        \
        HTTPBase.cpp                      \
        JPEGSource.cpp                    \
        MP3Extractor.cpp                  \
        MPEG2TSWriter.cpp                 \
        MPEG4Extractor.cpp                \
        MPEG4Writer.cpp                   \
        MediaBuffer.cpp                   \
        MediaBufferGroup.cpp              \
        MediaCodec.cpp                    \
        MediaCodecList.cpp                \
        MediaDefs.cpp                     \
        MediaExtractor.cpp                \
        MediaSource.cpp                   \
        MetaData.cpp                      \
        NuCachedSource2.cpp               \
        NuMediaExtractor.cpp              \
        OMXClient.cpp                     \
        OMXCodec.cpp                      \
        OggExtractor.cpp                  \
        SampleIterator.cpp                \
        SampleTable.cpp                   \
        SkipCutBuffer.cpp                 \
        StagefrightMediaScanner.cpp       \
        StagefrightMetadataRetriever.cpp  \
        SurfaceMediaSource.cpp            \
        ThrottledSource.cpp               \
        TimeSource.cpp                    \
        TimedEventQueue.cpp               \
        Utils.cpp                         \
        VBRISeeker.cpp                    \
        WAVExtractor.cpp                  \
        WVMExtractor.cpp                  \
        XINGSeeker.cpp                    \
        avc_utils.cpp                     \
        mp4/FragmentedMP4Parser.cpp       \
        mp4/TrackFragment.cpp             \

ifeq ($(BOARD_HAVE_QCOM_FM),true)
LOCAL_SRC_FILES+=                         \
        FMA2DPWriter.cpp
endif

LOCAL_C_INCLUDES:= \
        $(call include-path-for, alsp) \
        $(TOP)/frameworks/av/include/media/stagefright/timedtext \
        $(TOP)/frameworks/native/include/media/hardware \
        $(TOP)/external/flac/include \
        $(TOP)/external/tremolo \
        $(TOP)/external/openssl/include \
        $(TOP)/frameworks/av/media/libstagefright/al_libc \
        $(TOP)/frameworks/av/media/libstagefright/id3parser \
        $(TOP)/frameworks/av/media/libstagefright/mmminfo \
        $(TOP)/frameworks/av/include/alsp/inc \
        $(TOP)/frameworks/av/include/alsp/inc/common \
        $(TOP)/hardware/libhardware/include/hardware

ifneq ($(TI_CUSTOM_DOMX_PATH),)
LOCAL_C_INCLUDES += $(TI_CUSTOM_DOMX_PATH)/omx_core/inc
LOCAL_CPPFLAGS += -DUSE_TI_CUSTOM_DOMX
else
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/include/media/openmax
endif

ifeq ($(BOARD_USES_STE_FMRADIO),true)
LOCAL_SRC_FILES += \
        FMRadioSource.cpp                 \
        PCMExtractor.cpp
endif

ifeq ($(BOARD_USES_QCOM_HARDWARE),true)
LOCAL_SRC_FILES += \
        ExtendedWriter.cpp                \
        QCMediaDefs.cpp                   \
        QCOMXCodec.cpp                    \
        WAVEWriter.cpp                    \
        ExtendedExtractor.cpp             \
        QCUtilityClass.cpp

ifeq ($(TARGET_QCOM_MEDIA_VARIANT),caf)
LOCAL_C_INCLUDES += \
        $(TOP)/hardware/qcom/media-caf/mm-core/inc
else
LOCAL_C_INCLUDES += \
        $(TOP)/hardware/qcom/media/mm-core/inc
endif

ifeq ($(TARGET_QCOM_AUDIO_VARIANT),caf)
    ifeq ($(call is-board-platform-in-list,msm8660 msm7x27a msm7x30),true)
        LOCAL_SRC_FILES += LPAPlayer.cpp
    else
        LOCAL_SRC_FILES += LPAPlayerALSA.cpp
    endif
    ifeq ($(BOARD_USES_ALSA_AUDIO),true)
        ifeq ($(call is-chipset-in-board-platform,msm8960),true)
            LOCAL_CFLAGS += -DUSE_TUNNEL_MODE
            LOCAL_CFLAGS += -DTUNNEL_MODE_SUPPORTS_AMRWB
        endif
    endif
LOCAL_CFLAGS += -DQCOM_ENHANCED_AUDIO
LOCAL_SRC_FILES += TunnelPlayer.cpp
endif
endif


LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libcamera_client \
        libcrypto \
        libcutils \
        libdl \
        libdrmframework \
        libexpat \
        libgui \
        libicui18n \
        libicuuc \
        liblog \
        libmedia \
        libmedia_native \
        libsonivox \
        libssl \
        libstagefright_omx \
        libstagefright_yuv \
        libsync \
        libui \
        libutils \
        libvorbisidec \
        libz \

LOCAL_STATIC_LIBRARIES := \
        libstagefright_color_conversion \
        libstagefright_mp3dec \
        libstagefright_aacenc \
        libstagefright_matroska \
        libstagefright_timedtext \
        libvpx \
        libstagefright_mpeg2ts \
        libstagefright_httplive \
        libstagefright_id3 \
        libFLAC \
        libmmminfo \
        libid3parser

LOCAL_SRC_FILES += \
        chromium_http_stub.cpp
LOCAL_CPPFLAGS += -DCHROMIUM_AVAILABLE=1

LOCAL_SHARED_LIBRARIES += libstlport
include external/stlport/libstlport.mk

LOCAL_SHARED_LIBRARIES += \
        libstagefright_enc_common \
        libstagefright_avc_common \
        libstagefright_foundation \
        libdl \
        libalc

LOCAL_CFLAGS += -Wno-multichar

ifeq ($(BOARD_USE_SAMSUNG_COLORFORMAT), true)
LOCAL_CFLAGS += -DUSE_SAMSUNG_COLORFORMAT

# Include native color format header path
LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung/exynos4/hal/include \
	$(TOP)/hardware/samsung/exynos4/include

endif

ifeq ($(BOARD_USE_TI_DUCATI_H264_PROFILE), true)
LOCAL_CFLAGS += -DUSE_TI_DUCATI_H264_PROFILE
endif

LOCAL_CFLAGS += -DTURN_ON_MIDDLEWARE_FLAG

LOCAL_MODULE:= libstagefright

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
