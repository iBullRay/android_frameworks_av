/*  File: ActAudioDownmix.h     $Revision:   1.0 $  */

/****************************************************************************
 ;  File:   downmix_cz.h
 ;  by chenzhan
 ;
 ;  History:
 ;      2009/03/20      Created
 ;***************************************************************************/
#ifndef ACTAUDIODOWNMIX_H
#define ACTAUDIODOWNMIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "actal_posix_dev.h"

#define DOWNMIX_BIT32
//#define DOWNMIX_BIT24
//#define DOWNMIX_BIT16

//#define  DOWNMIX_GNRNG

/* # audio coding modes */
#define DOWNMIX_NACMOD 9
/**** General system equates ****/
/* max # channels */
#define DOWNMIX_NCHANS 6
/* max # full bw channels */
//#define DOWNMIX_NFCHANS 5
/* max # output channels */
#define DOWNMIX_NPCMCHANS 6

/**** Filter bank equates ****/
/* block size */
#define DOWNMIX_N 2048

/**** DSP type definitions ****/
typedef short downmix_int16; /* DSP integer */
typedef unsigned short downmix_uint16; /* DSP unsigned integer */

#ifdef DOWNMIX_BIT16
#define DOWNMIX_BitNum_word 16
#define DOWNMIX_UNITY 0x7fff
#define DOWNMIX_M3DB 23170
#define DOWNMIX_M6DB 16384

typedef short downmix_int32; /* DSP fixed-point fractional */
typedef int downmix_int64;

#endif

#ifdef DOWNMIX_BIT24
#define DOWNMIX_BitNum_word 24
#define DOWNMIX_UNITY 0x7fffff
#define DOWNMIX_M3DB 5931642
#define DOWNMIX_M6DB 4194304

typedef int downmix_int32; /* DSP fixed-point fractional */
typedef __int64 downmix_int64;

#endif

#ifdef DOWNMIX_BIT32
#define DOWNMIX_BitNum_word 32
#define DOWNMIX_UNITY 0x7fffffff
#define DOWNMIX_M3DB 1518500352
#define DOWNMIX_M6DB 1073741824

typedef int downmix_int32; /* DSP fixed-point fractional */
#ifdef _WIN32
typedef __int64 downmix_int64;
#else
typedef long long downmix_int64;
#endif

#endif

/**** Enumerations ****/
enum {
    DOWNMIX_MODE11,
    DOWNMIX_MODE10,
    DOWNMIX_MODE20,
    DOWNMIX_MODE30, /* audio coding mode */
    DOWNMIX_MODE21,
    DOWNMIX_MODE31,
    DOWNMIX_MODE22,
    DOWNMIX_MODE32,
    DOWNMIX_MODE32ACT
};
enum {
    DOWNMIX_DUAL_STEREO, DOWNMIX_DUAL_LEFTMONO, /* dual mono downmix mode */
    DOWNMIX_DUAL_RGHTMONO, DOWNMIX_DUAL_MIXMONO
};

/****  channel ordering equates ****/
/* DOWNMIX_LEFT channel */
#define DOWNMIX_LEFT 0
/* center channel */
#define DOWNMIX_CNTR 1
/* right channel */
#define DOWNMIX_RGHT 2
/* DOWNMIX_LEFT surround channel */
#define DOWNMIX_LSUR 3
/* right surround channel */
#define DOWNMIX_RSUR 4
/* low frequency effects channel */
#define DOWNMIX_LFE 5
/* mono surround channel */
#define DOWNMIX_MSUR 3
/* channel not in use */
#define DOWNMIX_NONE -1

#define DOWNMIX_MYLIMIT(a,b) (downmix_int32)((((downmix_int64)(a) + (downmix_int64)(b))>DOWNMIX_UNITY)?DOWNMIX_UNITY:((((downmix_int64)(a) + (downmix_int64)(b))<(-DOWNMIX_UNITY-1))?(-DOWNMIX_UNITY-1):((downmix_int64)(a) + (downmix_int64)(b))))
/* absolute value */
#define downmix_abs(a)      (((a)>=0)?(a):(-(a)))

#define downmix_malloc actal_malloc
#define downmix_memset actal_memset
#define downmix_free(p) { if(p) { actal_free((p)); (p)=NULL; } }

typedef struct {
    downmix_int16 acmod;
    downmix_int16 outmod;
    downmix_int16 cmixlev;
    downmix_int16 surmixlev;
    downmix_int16 lfeon;
    downmix_int16 outlfe;
    downmix_int16 dualmod;
    downmix_int32 *downmix_buf[DOWNMIX_NCHANS];

} downmix_state;

downmix_state *downmix_open(void);

downmix_int32 downmix_set(downmix_state *dommix_str,
        downmix_int32 *downmix_buf_left, downmix_int32 *downmix_buf_right);

downmix_int32 downmix_run(downmix_state *dommix_str, downmix_int32 *tcbuf,
        downmix_int32 frame_len, downmix_int32 channum);

downmix_int32 downmix_close(downmix_state *dommix_str);

#ifdef __cplusplus
}
#endif
#endif