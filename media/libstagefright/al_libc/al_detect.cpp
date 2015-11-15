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

#define LOG_TAG "AL_DETECT"
#include <utils/Log.h>

#include <actal_posix_dev.h>
#include "SinoDetect.h"

int actal_encode_detect(const char *src, char *encoding) {
    int sd_encoding = SinoDetect::OTHER;
    SinoDetect sd;

    if ((src == NULL) || (strlen(src) == 0)) {
        return -1;
    }

    sd_encoding = sd.detect_encoding((unsigned char*) src);
    switch (sd_encoding) {
    case SinoDetect::BIG5:
        strcpy(encoding, "Big5");
        break;
    case SinoDetect::EUC_KR:
        strcpy(encoding, "EUC-KR");
        break;
    case SinoDetect::EUC_JP:
        strcpy(encoding, "EUC-JP");
        break;
    case SinoDetect::SJIS:
        strcpy(encoding, "shift-jis");
        break;
    case SinoDetect::GBK:
    default:
        strcpy(encoding, "gbk");
        break;
    }

    return 0;
}
