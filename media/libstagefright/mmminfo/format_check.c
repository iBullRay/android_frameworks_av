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

#include "format_check.h"

int format_check(storage_io_t* input, const char* mime) {
    int ret = 0, flag = 0, len = 5;
    unsigned int* tmp;
    char *ext = (char *)mime;
    char buf[16];    
    
    tmp = (unsigned int*)buf;
    if ((ext == NULL) || (input == NULL)) {
        actal_error("format_check Err:buffer is empty!\n");
        return -1;
    }
    input->seek(input, 0, SEEK_SET);
    len = input->read(buf, 1, 16, input);
    if (len < 16) {
        actal_error("format_check read file header Err %x\n",len);
        return -1;
    }
    while((tmp[0] & 0xffffff) == 0x334449) {
        int tagsize;
        /* high bit is not used */
        if (((buf[6]|buf[7]|buf[8]|buf[9]) & 0x80) == 0x80) {
            actal_error("Bad ID3, skip ID3..\n");
            break;
        }
        
        tagsize =( (buf[6] << 21) | (buf[7] << 14)
                   | (buf[8] <<  7) | (buf[9] ));
        flag += (tagsize+10);

        input->seek(input, flag, SEEK_SET);
        /* get another 10bytes for format decision */
        len = input->read(buf, 1, 10, input);
        if (len != 10) {
            actal_error("format_check read ID3 Err\n");
            return -1;
        }
    }
    if (flag > 0) {
        if ((tmp[0] &0xffffff )== 0x2b504d) {
            strcpy(ext, PARSER_EXT_MPC);
        } else if (tmp[0] == 0x43614c66) {
            strcpy(ext, PARSER_EXT_FLAC);
        } else if (tmp[0] == 0x2043414d) {
            strcpy(ext, PARSER_EXT_APE);
        } else if (adts_aac_check(input) == 1) {
            strcpy(ext, PARSER_EXT_AAC);
        } else {
            input->seek(input, flag, SEEK_SET);
            flag = mp3check(input);
            if (flag == 0) {
                strcpy(ext, PARSER_EXT_MP3);
                printf("format check other1 ext:%s\n",ext);
                goto ErrEXit;
            } else {
                ret = -1;
                actal_error("format check1 Err!(ID3)  ext:%s\n",ext);
                goto ErrEXit;
            }
        }
        printf("format_check ID3  ext:%s\n",ext);
        goto ErrEXit;
    } else if (tmp[0] == 0x75B22630) {
        input->seek(input, 0, SEEK_SET);
        flag = wmaflag(input);
        if (flag == 0) {
            strcpy(ext, "wmv");
        } else if (flag == 1) {
            strcpy(ext, PARSER_EXT_WMA);
        } else {
            actal_error("format_check read ASF header Err\n");
            return -1;
        }        
        goto ErrEXit;
    } else if (tmp[0] == 0x5367674f) {
        input->seek(input,28,SEEK_SET);
        len = input->read(tmp, 1, 4,input);
        if (len != 4) {
            actal_error("format_check read SPX header Err\n");
            return -1;
        }
        if (tmp[0] == 0x65657053) {
            strcpy(ext, "SPX");
        } else if (tmp[0] == 0x726f7601) {
            strcpy(ext, PARSER_EXT_OGG);
        } else {
            strcpy(ext, "ogm");
        }
        goto ErrEXit;
    } else if ((tmp[2] & 0xffffff) == 0x564d41) {
        ret = -1;
        goto ErrEXit;
    } else if (tmp[0] == 0x46464952) {
        char sExt[4];
        sExt[0] = (buf[8] >= 'a') ? (buf[8] - 32) : buf[8]; // 32 = 'a' - 'A'
        sExt[1] = (buf[9] >= 'a') ? (buf[9] - 32) : buf[9];
        sExt[2] = (buf[10] >= 'a') ? (buf[10] - 32) : buf[10];
        sExt[3] = 0x20;

        if (memcmp(sExt, "AVI", 3) == 0) {
            strcpy(ext, "avi");
        } else if (strncmp(&buf[8], "CDXAfmt", 7) == 0) {
            strcpy(ext, "mpg");
        } else {
            flag = dts_check(input);
            if (flag == 1) {
                strcpy(ext, PARSER_EXT_DTS);
                actal_printf("format_check DTS 1th\n");
            } else {
                strcpy(ext, PARSER_EXT_WAV);
            }
        }
        goto ErrEXit;
    } else if (tmp[1] == 0x70797466) {
        input->seek(input, 0, SEEK_SET);
        if (aacflag(input) == 0) {
            strcpy(ext, "mp4");
            actal_printf("format_check aacflag\n");
        } else {
            int is_plus = aacplusflag(input);
            if (is_plus <= 0) {
                ret = -1;
                goto ErrEXit;
            }

            if (is_plus == 1 ) {
                strcpy(ext, PARSER_EXT_ALAC);
            } else {
                strcpy(ext, PARSER_EXT_AAC);
                
            }
        }
        actal_printf("%s %d  ext:%s\n",__FILE__,__LINE__,ext);
        goto ErrEXit;
    } else if (tmp[0] == 0x464d522e) {
        input->seek(input, 0, SEEK_SET);
        flag = rm_check(input);
        if (flag == 1) {
            strcpy(ext, PARSER_EXT_RMA);
        } else {
            strcpy(ext, "rm");
        }
        goto ErrEXit;
    } else if ((tmp[0] &0xffffff )== 0x2b504d) {
        strcpy(ext, PARSER_EXT_MPC);
        goto ErrEXit;
    } else if (((tmp[0] & 0xffff) == 0x770b) ||((tmp[0] & 0xffff) == 0x0b77)) {
        strcpy(ext, PARSER_EXT_AC3);
        goto ErrEXit;
    } else if (tmp[0] == 0x36759057) {
        strcpy(ext, PARSER_EXT_AA);
        goto ErrEXit;
    } else if ((tmp[0] & 0xfffffff) == 0x1564c46) {
        strcpy(ext, "flv");
        goto ErrEXit;
    } else if ((tmp[0] == 0xba010000) || (tmp[0] == 0xb3010000)) {
        strcpy(ext, "mpg");
        goto ErrEXit;
    } else if (tmp[0] == 0x4d412123) {
        strcpy(ext, PARSER_EXT_AMR);
        goto ErrEXit;
    } else if (tmp[0] == 0xa3df451a) {
        strcpy(ext, "mkv");
        goto ErrEXit;
    }
    if ((tmp[1] == 0x70797466)
            ||(tmp[1] == 0x766f6f6d)
            ||(tmp[1] == 0x7461646d)
            ||(tmp[1] == 0x666f6f6d)
            ||(tmp[1] == 0x6172666d)
            ||(tmp[1] == 0x6e696470)
            ||(tmp[1] == 0x65657266)
            ||(tmp[1] == 0x70696b73)
            ||(tmp[1] == 0x6174656d)
            ||(tmp[1] == 0x746f6e70)
            ||(tmp[1] == 0x65646977)) {
        strcpy(ext, "mp4");
        goto ErrEXit;
    }

    if (tmp[0] == 0x2043414d) {
        strcpy(ext, PARSER_EXT_APE);
        goto ErrEXit;
    } else if (tmp[0] == 0x43614c66) {
        strcpy(ext, PARSER_EXT_FLAC);
        goto ErrEXit;
    } else if (!(memcmp(buf, "FWS", 3)) || !(memcmp(buf, "CWS", 3))) {
        ret = -1;
        goto ErrEXit;
    } else if (!(memcmp(buf, "FORM", 4))) {
        strcpy(ext, PARSER_EXT_AIFF);
        goto ErrEXit;
    } else {
        input->seek(input, 0, SEEK_SET);
        flag = mp3check(input);
        if (flag == 0) {
            strcpy(ext, PARSER_EXT_MP3);
            actal_printf("format check other ext:%s\n",ext);
            goto ErrEXit;
        }
        input->seek(input, 0, SEEK_SET);
        if (ts_check(input) == 1) {
            strcpy(ext, "ts");
            goto ErrEXit;
        }
        input->seek(input, 0, SEEK_SET);
        flag = adts_aac_check(input);
        if (flag == 1) {
            strcpy(ext, PARSER_EXT_AAC);
            actal_printf("format check other ext:%s\n",ext);
            goto ErrEXit;
        }
        flag = dts_check(input);
        if (flag == 1) {
            strcpy(ext, PARSER_EXT_DTS);
            actal_printf("format check other ext:%s\n",ext);
            goto ErrEXit;
        }       
        ret = -1;
        actal_error("format undistinguish!!  ext:%s\n",ext);
    }
ErrEXit:
    actal_printf("format_Check  ext:%s\n",ext);
    return ret;
}
