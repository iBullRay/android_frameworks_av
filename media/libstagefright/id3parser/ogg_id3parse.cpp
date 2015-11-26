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

namespace android {
int find_ogg_Tag(void *fp, char * ogg_TagInfo, int length) {
    int i;
    int j = 0;
    int k = 0;
    int vorbis_address = 0;
    int len;
    char * tmp = (char *) malloc(ID3_SECTORSIZE);
    if (NULL == tmp) {
        return -1;
    }
    for (i = 0; i < 8; i++) {
        ID3_fseek(fp, i * ID3_SECTORSIZE, SEEK_SET);
        ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fp);
        for (j = 0; j < ID3_SECTORSIZE; j++) {
            if (tmp[j] != ogg_TagInfo[k]) {
                k = 0;
            } else {
                k++;
                if (k >= length) {
                    vorbis_address = (i * ID3_SECTORSIZE) + j + 1;
                    free(tmp);
                    return vorbis_address;
                }
            }
        }
    }
    free(tmp);
    return vorbis_address;
}

void get_ogg_item(void *fd, const char *string, int addr, id3_item_info_t* it_info) {
    int i;
    int j;
    int k = 0;
    int len;
    char * tmp;
    int tempval;

    if ((addr <= 0) || (string == NULL)) {
        return;
    }
    len = (int) strlen(string);
    tmp = (char *) malloc(ID3_SECTORSIZE);
    if (tmp == NULL) {
        return;
    }
    for (i = 0; i < (8 - (addr / ID3_SECTORSIZE)); i++) {
        ID3_fseek(fd, addr + (i * ID3_SECTORSIZE), SEEK_SET);
        ID3_fread(tmp, sizeof(char), ID3_SECTORSIZE, fd);

        for (j = 0; j < ID3_SECTORSIZE; j++) {
            if (UPPER(tmp[j]) != string[k]) {
                k = 0;
            } else {
                k++;
                if (k >= len) {
                    tempval = j - len - 4;
                    if (tempval >= 0) {
                        memcpy(&(it_info->length), tmp + tempval + 1, 4);
                    } else {
                        ID3_fseek(fd, addr + (i * ID3_SECTORSIZE) + (j - len - 4) + 1, SEEK_SET);
                        ID3_fread(&(it_info->length), sizeof(char), 4, fd);
                    }
                    it_info->length -= len;
                    if ((it_info->length <= 0) || (it_info->length > 0xC000)) {
                        k = 0;
                        it_info->length = 0;
                        continue;
                    }
                    it_info->content = (char*) malloc((unsigned int) (it_info->length + 1));
                    if (NULL == it_info->content) {
                        free(tmp);
                        return;
                    }

                    if (j < (ID3_SECTORSIZE - it_info->length)) {
                        memcpy(it_info->content, tmp + j + 1, (unsigned int) it_info->length);
                    } else {
                        ID3_fseek(fd, addr + (i * ID3_SECTORSIZE) + j + 1, SEEK_SET);
                        ID3_fread(it_info->content, sizeof(char), (unsigned int) it_info->length, fd);
                    }
                    it_info->content[it_info->length] = '\0';
                    it_info->encoding = ENCODING_UTF8;
                    free(tmp);
                    return;
                }
            }
        }
    }
    free(tmp);
    return;
}

void get_oggtag(void *fd, id3_info_t* Tag) {
    int addr;
    int tmp;
    char ogg_TagInfo[] = { 0x03, 0x76, 0x6f, 0x72, 0x62, 0x69, 0x73 };

    addr = find_ogg_Tag(fd, ogg_TagInfo, 7);
    if (addr <= 0) {
        return;
    }
    ID3_fseek(fd, addr, SEEK_SET);
    ID3_fread(&tmp, sizeof(char), 4, fd);
    addr += tmp + 8;
    get_ogg_item(fd, "TITLE=", addr, &Tag->title);
    get_ogg_item(fd, "ARTIST=", addr, &Tag->author);
    get_ogg_item(fd, "ALBUM=", addr, &Tag->album);
    get_ogg_item(fd, "GENRE=", addr, &Tag->genre);
    get_ogg_item(fd, "DATE=", addr, &Tag->year);
    get_ogg_item(fd, "COMMENT=", addr, &Tag->comment);
    get_ogg_item(fd, "ANDROID_LOOP=", addr, &Tag->autoLoop);

#if __TRACK_NUM__ > 0
    get_ogg_item(fd, "TRACKNUMBER=", addr, &Tag->track);
#endif
}

void get_ogg_audio_info(void *fd, id3_info_total* info) {
    memset(info, 0, sizeof(id3_info_total));
    get_oggtag(fd, &info->tag);
}

}
