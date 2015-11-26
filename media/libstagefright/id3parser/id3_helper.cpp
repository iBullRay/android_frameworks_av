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

#include "id3parse.h"
#include "music_parser_lib_dev.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"

//#define LOG_NDEBUG 0
#define LOG_TAG "ID3parser"
#include <utils/Log.h>

namespace android {
#define EXT_LEN 10
void Item_UTF8_To_UTF16(id3_item_info_t *pItemInfo);
void change_ext_standard(char *min, char *cap);
static void ensure_info(id3_item_info_t *info);

size_t codec_convert(char *from_charset, char *to_charset, char *inbuf,
        size_t inlen, char *outbuf, size_t outlen) {
    size_t length = outlen;
    char *pin = inbuf;
    char *pout = outbuf;
    UErrorCode status = U_ZERO_ERROR;

    UConverter *conv = ucnv_open(from_charset, &status);
    if (U_FAILURE(status)) {
        ALOGE("could not create UConverter for %s\n", from_charset);
        return -1;
    }
    UConverter *destConv = ucnv_open(to_charset, &status);
    if (U_FAILURE(status)) {
        ALOGE("could not create UConverter for  for %s\n", to_charset);
        ucnv_close(conv);
        return -1;
    }
    ucnv_convertEx(destConv, conv, &pout, pout + outlen,
            (const char **)&pin, (const char *)pin + inlen, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
    if (U_FAILURE(status)) {
        ALOGE("ucnv_convertEx failed: %d\n", status);
    } else {
        *pout = 0;
    }
    ucnv_close(conv);
    ucnv_close(destConv);
    return 0;
}

void getgenre(char c, id3_item_info_t* temp) {
    int i;
    i = c & 0xff;
    temp->content = (char*) malloc(16);
    sprintf(temp->content, "%d", i);
    temp->length = 16;
}

void freeItem(id3_item_info_t *item) {
    if (item->content != NULL) {
        free(item->content);
    }
    memset(item, 0, sizeof(id3_item_info_t));
}

void freeallmemory(id3_info_total *info) {
    freeItem(&info->tag.author);
    freeItem(&info->tag.composer);
    freeItem(&info->tag.album);
    freeItem(&info->tag.genre);
    freeItem(&info->tag.year);
    freeItem(&info->tag.title);
    freeItem(&info->tag.comment);
    freeItem(&info->tag.autoLoop);
#if __TRACK_NUM__ > 0

    freeItem(&info->tag.track);
#endif
}

void TransToLittleEnd(char *buffer, int bufferLen) {
    int i;
    char tmp;
    if (buffer == NULL) {
        return;
    }
    for (i = 0; i < bufferLen; i += 2) {
        tmp = buffer[i];
        buffer[i] = buffer[i + 1];
        buffer[i + 1] = tmp;
    }
    return;
}

void item_itoa(id3_item_info_t *pitem) {
    int num;
    char buffer[10];
    int offset;
    if (pitem == NULL) {
        return;
    }
    if (pitem->length != 4) {
        return;
    }

    memcpy(&num, pitem->content, 4);
    num = num % 10000;
    if (num > 1000) {
        offset = 0;
    } else if (num > 100) {
        offset = 1;
    } else if (num > 10) {
        offset = 2;
    } else {
        offset = 3;
    }

    memset(buffer, 0, 10);
    buffer[0] = (char) ((num / 1000) + 0x30);
    num = num % 1000;
    buffer[1] = (char) ((num / 100) + 0x30);
    num = num % 100;
    buffer[2] = (char) ((num / 10) + 0x30);
    buffer[3] = (char) ((num % 10) + 0x30);

    buffer[0] = buffer[offset];
    buffer[1] = buffer[offset + 1];
    buffer[2] = buffer[offset + 2];
    buffer[3] = buffer[offset + 3];

    freeItem(pitem);
    assert(strlen(buffer) < 10);
    pitem->length = (int) strlen(buffer);
    pitem->encoding = ENCODING_NORMAL;
    pitem->content = (char *)malloc((unsigned int) (pitem->length + 1));
    if (NULL == pitem->content) {
        return;
    }
    memset(pitem->content, 0, (unsigned int) (pitem->length + 1));
    memcpy(pitem->content, buffer, (unsigned int) pitem->length);

}

int mlang_unicode_to_utf8 (unsigned short *unicode, int unicode_len, char *utf8, int *putf8_len) {
    int i = 0;
    int outsize = 0;
    int charscount = 0;
    char *tmp = utf8;
#if 0
    codec_convert("unicode", "UTF-8", (char *)unicode, unicode_len, (char *)utf8, *putf8_len );
#else 
    charscount = unicode_len / sizeof(uint16_t);
    unsigned short unicode_tmp;
    for (i = 0; i < charscount; i++) {
        unicode_tmp = unicode[i];
        if (unicode_tmp >= 0x0000 && unicode_tmp <= 0x007f) {
            *tmp = (uint8_t)unicode_tmp;
            tmp += 1;
            outsize += 1;
        } else if (unicode_tmp >= 0x0080 && unicode_tmp <= 0x07ff) {
            *tmp = 0xc0 | (unicode_tmp >> 6);
            tmp += 1;
            *tmp = 0x80 | (unicode_tmp & 0x003f);
            tmp += 1;
            outsize += 2;
        } else if (unicode_tmp >= 0x0800 && unicode_tmp <= 0xffff) {
            *tmp = 0xe0 | (unicode_tmp >> 12);
            tmp += 1;
            *tmp = 0x80 | ((unicode_tmp >> 6) & 0x003f);
            tmp += 1;
            *tmp = 0x80 | (unicode_tmp & 0x003f);
            tmp += 1;
            outsize += 3;
        }
    }

    *tmp = '\0';
    *putf8_len = outsize;
#endif    
    return 0;
}

int wstrlen(char *buffer, int bufferLen) {
    int retVal;
    short *wchar;
    int i;
    int len = bufferLen / 2;
    wchar = (short *) buffer;
    for (i = 0; i < len; i++) {
        if (*wchar == 0) {
            break;
        }
        wchar++;
    }
    if (i <= len) {
        retVal = i * 2;
    } else {
        retVal = 0;
    }
    return retVal;
}

int Unicode2utf8(char *str, int length) {
    char * temp;
    int stringlen;
    int dst_len;
    int retVal;

    if ((str == NULL) || (length < 0)) {
        return -1;
    }

    temp = (char *) malloc(512);
    if (NULL == temp) {
        return -1;
    }
    dst_len = 512;
    retVal = mlang_unicode_to_utf8((unsigned short *)str, length, temp, &dst_len);
    if (retVal < 0) {
        free(temp);
        return -1;
    }

    memcpy(str, (const char*)temp, (size_t)(dst_len+1));
    free(temp);
    return 0;
}

int is_modifiy_utf8 (char *utf8, int length) {
    int i = 0;
    unsigned char chr;
    char *pt = utf8;

    while (i < length) {
        chr = *(pt++);
        i++;
        // Switch on the high four bits.
        switch (chr >> 4) {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07: 
            {
                // Bit pattern 0xxx. No need for any extra bytes.
                break;
            }
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0f:
            {
                /*
                 * Bit pattern 10xx or 1111, which are illegal start bytes.
                 * 
                 */
                return -1;
            }
            case 0x0e: 
            {
                // Bit pattern 1110, so there are two additional bytes.
                chr = *(pt++);
                i++;
                if ((chr & 0xc0) != 0x80) {
                    return -1;
                }
            }
            case 0x0c:
            case 0x0d: 
            {
                // Bit pattern 110x, so there is one additional byte.
                chr = *(pt++);
                i++;
                if ((chr & 0xc0) != 0x80) {
                    return -1;
                }
                break;
            }
        }
    }
    return 0;
}

void transItemEncoding(id3_item_info_t *targetItem, id3_item_info_t *sourceItem, char encoding) {
    int length;
    int ret = 0;

    if ((NULL == sourceItem) || (NULL == targetItem) || (sourceItem->length <= 0)) {
        return;
    }
    memset(targetItem, 0, sizeof(id3_item_info_t));

    length = ID3_SECTORSIZE * 3;
    targetItem->content = (char *) malloc((unsigned int) length);
    if (NULL == targetItem->content) {
        return;
    }
    memset(targetItem->content, 0, (unsigned int) length);

    if (ENCODING_UTF8 == sourceItem->encoding) {
        if (sourceItem->content == NULL) {
            targetItem->length = 0;
            free(targetItem->content);
            targetItem->content = NULL;
            return;
        }

        ret = is_modifiy_utf8((char *)sourceItem->content, sourceItem->length);
        if (ret != 0) {
            char *pt =  targetItem->content;
            targetItem->length = sourceItem->length + 2;
            *pt    = 0xcf;
            pt++;
            *pt    = 0xcf;
            pt++;
            memcpy(pt, sourceItem->content, (unsigned int) sourceItem->length);
            pt[sourceItem->length] = 0;
        } else {
            memcpy(targetItem->content, sourceItem->content, (unsigned int) sourceItem->length);
            targetItem->length = sourceItem->length;
        }
        ALOGV("ENCODING_UTF8 (%s)(%s)\n", sourceItem->content, targetItem->content);
        
    } else if (ENCODING_NORMAL == sourceItem->encoding) {
        char *pt = sourceItem->content;
        ALOGV("ENCODING_NORMAL begin (%s)\n", sourceItem->content);
        ret = 0;
        for (int i = 0; i < sourceItem->length; i++,pt++) {
            // Is the string all assic,
            if ((*pt >= 0x80) || (*pt < 0)) {
                ret = -1;
                break;
            }
        } 
        pt =  targetItem->content;
        targetItem->length = sourceItem->length;
        if (ret < 0) {
            // these are not only inclue ASSIC.
            targetItem->length = sourceItem->length + 2;
            *pt = 0xcf;
            pt++;
            *pt = 0xcf;
            pt++;
        }
        memcpy(pt, sourceItem->content, (unsigned int) sourceItem->length);
        pt[sourceItem->length] = 0;
        ALOGV("ENCODING_NORMAL end (%s)\n", pt);
    } else if (ENCODING_UNICODE == sourceItem->encoding) {
        ret = mlang_unicode_to_utf8((unsigned short *)sourceItem->content, sourceItem->length, targetItem->content, &targetItem->length);
        if ((ret < 0) || (length < 0)) {
            targetItem->length = 0;
            free(targetItem->content);
            targetItem->content = NULL;
            ALOGV("target content NULL, ENCODING_UNICODE\n");
            return;
        }
        ALOGV("ENCODING_UNICODE (%s)(%s)\n", sourceItem->content, targetItem->content);
    } else {
        ALOGD("unknown ENCODING\n");
        return;
    }

}

void transItem(id3_item_info_t *pitem, char encoding) {
    id3_item_info_t tmpItem;

    memset(&tmpItem, 0, sizeof(tmpItem));
    transItemEncoding(&tmpItem, pitem, encoding);
    freeItem(pitem);
    memcpy(pitem, &tmpItem, sizeof(tmpItem));
}

int get_audio_id3_info(ID3file_t* fp, char *fileinfo, id3_info_total* info) {
    id3_item_info_t *id3tag;
    int i;
    int value = 0;
    int length;
    void * source = (void *)fp;

    if((strcmp(fileinfo, PARSER_EXT_MP3) == 0) ||(strcmp(fileinfo, "audio/mpeg") == 0)) {
        get_mp3_audio_info(source, info);
    } else if(( strcmp(fileinfo, PARSER_EXT_WMA) == 0) ||(strcmp(fileinfo, "audio/x-ms-wma") == 0)) {
        get_wma_audio_info(source, info);
    } else if(( strcmp(fileinfo, PARSER_EXT_OGG) == 0)||(strcmp(fileinfo, "audio/ogg") == 0)) {
        get_ogg_audio_info(source, info);
    } else if(( strcmp(fileinfo, PARSER_EXT_FLAC) == 0)||(strcmp(fileinfo, "audio/x-flac") == 0)) {
        get_flac_audio_info(source, info);
    } else {
        get_ape_audio_info(source, info);
    }

    id3tag = &info->tag.author;

#if 1
    for (i = 0; i < 8; i++) {
        ensure_info(id3tag);
        transItem(id3tag, ENCODING_UTF8);
        id3tag++;
    }
#endif    
    return 0;
}

void change_ext_standard(char *min, char *cap) {
    unsigned int i;
    char c;

    if ((NULL == min) || (NULL == cap)) {
        return;
    }

    for (i = 0, c = *min; (c != 0)&&(i < (EXT_LEN-1)); min++, c = *min) {
        if ((c > 0x60) && (c < 0x7b)) {
            cap[i] = c - 32;
        } else {
            cap[i] = c;
        }
        i++;
    }
    cap[i] = '\0';
}

static void ensure_info(id3_item_info_t *info) {
    char * tmp;
    int i;

    if (info == NULL) {
        return;
    }
    if (info->content == NULL) {
        memset(info, 0, sizeof(id3_item_info_t));
        return;
    }

    if ((info->encoding == ENCODING_NORMAL) || (info->encoding == ENCODING_UTF8)) {
        info->length = (int)strlen(info->content);
        i = info->length - 1;
        while (i >= 0) {
            if (info->content[i] != 0x20) {
                break;
            }
            i--;
        }
        if (i < 0) {
            freeItem(info);
            return;
        }
        info->content[i + 1] = '\0';
        info->length = (int)strlen(info->content);
    }
    return;
}

} // namespace android
