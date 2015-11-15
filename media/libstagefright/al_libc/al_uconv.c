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

#define LOG_TAG "AL_UCONV"
#include <utils/Log.h>
#include "uconv_dev.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"

int actal_check_utf8(const char *utf8, int length) {
    int i = 0;
    unsigned char ch;
    const char *pt = utf8;

    while (i < length) {
        // Switch on the high four bits.
        ch = pt[i++];
        if ((ch & 0x80) == 0) {
            // Bit pattern 0xxx0000. No need for any extra bytes.
            continue;
        } else if (((ch & 0xc0) == 0x80) || ((ch & 0xf0) == 0xf0)) {
            // Bit pattern 10xx or 1111, which are illegal start bytes.
            return -1;
        } else {
            // if Bit pattern 1110, there are two additional bytes.
            // if Bit pattern 110x, there is one additional byte.
            if ((ch & 0xe0) == 0xe0) {
                ch = pt[i++];
                if ((ch & 0xc0) != 0x80) {
                    return -1;
                }
            }

            ch = pt[i++];
            if ((ch & 0xc0) != 0x80) {
                return -1;
            }
        }
    }

    return 0;
}

int actal_convert_ucnv(char *from_charset, char *to_charset, const char *inbuf, int inlen,
        char *outbuf, int outlen) {
    int ret = 0;
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
            &inbuf, inbuf + inlen, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
    if (U_FAILURE(status)) {
        ALOGE("ucnv_convertEx failed: %d\n", status);
        ret = -1;
    } else {
        *pout = 0;
    }

    ucnv_close(conv);
    ucnv_close(destConv);
    return ret;
}
