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

#ifndef __AL_LIBC_H__
#define __AL_LIBC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define INLINE inline

typedef int64_t mmm_off_t;

void *actal_memcpy(void *, const void *, int32_t);
void *actal_memset(void *, int32_t, int32_t);

void *actal_malloc(int32_t);
void actal_free(void *);
void *actal_malloc_dma(int32_t, int32_t *);
void actal_free_dma(void *);
void *actal_malloc_wt(int32_t, int32_t *);
void actal_free_wt(void *);
void actal_cache_flush(void *, int);
void actal_cache_env(void *, int);
void *actal_malloc_uncache(int32_t, int32_t *);
void actal_free_uncache(void *);
int actal_get_phyaddr(void *);
void * actal_get_virtaddr(int );
void actal_dump(int *, int32_t);

int64_t actal_get_ts(void);
int32_t actal_printf(const char *format, ...);
int32_t actal_error(const char *format, ...);
int32_t actal_info(const char *format, ...);
void actal_sleep_ms(int32_t);
int actal_get_icinfo(void);
int actal_check_utf8(const char *utf8, int length);
int actal_convert_ucnv(char *from_charset, char *to_charset, const char *inbuf, int inlen,
        char *outbuf, int outlen);
int actal_encode_detect(const char *src, char *encoding);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __AL_LIBC_H__
