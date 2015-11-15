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

#ifndef __ACTAL_LIBC_DEV_H__
#define __ACTAL_LIBC_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef long long int64_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;

#include "./common/al_libc.h"

#define NULL 0
#define TRUE 1
#define FALSE 0

typedef int FILE;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE *fopen(const char *, const char *);
int fread(void *, int, int, FILE *);
int fwrite(void *, int, int, FILE *);
int fseek(FILE *, int, int);
int ftell(FILE *);
int feof(FILE *);
int fprintf(FILE *, const char *, ...);
char *fgets(char *, int, FILE *);
int fclose(FILE *);

void *memcmp(const void *, const void *, int);
void *memmove(void *, const void *, int);

char *strcpy(char *, const char *);
char *strncpy(char *, const char *, int);
char *strcat(char *, const char *);
char *strchr(const char *, int);
char *strstr(const char *, const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, int);
int strlen(const char *);
int sprintf(char *, const char *, ...);
int sscanf(const char *, const char *, ... );
int atoi(const char *);
int printf(const char *, ...); 

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __ACTAL_LIBC_DEV_H__
