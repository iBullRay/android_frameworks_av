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

#ifndef __UCONV_DEV_H__
#define __UCONV_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

int convert_ucnv(char *from_charset, char *to_charset, char *inbuf, int inlen,
        char *outbuf, int outlen);
int check_valid_utf8(char *utf8, int length);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif
