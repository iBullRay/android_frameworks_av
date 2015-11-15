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

#define LOG_TAG "AL_LIBC"
#ifdef ANDROID
#include <utils/Log.h>
#endif
#ifndef ANDROID
#define ALOGV(fmt, args...) do{printf("V " fmt "\n", ##args);}while(0)
#define ALOGD(fmt, args...) do{printf("D " fmt "\n", ##args);}while(0)
#define ALOGI(fmt, args...) do{printf("I " fmt "\n", ##args);}while(0)
#define ALOGW(fmt, args...) do{printf("W " fmt "\n", ##args);}while(0)
#define ALOGE(fmt, args...) do{printf("E " fmt "\n", ##args);}while(0)
#endif

#include "actal_posix_dev.h"
#include <ion/ion.h>

#define ALIGN_BYTES 4096
#define ALIGN_MASK (ALIGN_BYTES - 1)

#define PRINT_BUF_SIZE 1024

struct actal_mem {
    int fd;
    struct ion_handle * handle;
    int len;
    void *ptr;
    int map_fd;
    int phy_add;
    struct actal_mem *next;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_fd = 0;
static int s_pid = 0;
static struct actal_mem *s_top_p = NULL, *s_current_p = NULL;
#define _ALSP_DEBUG_
int actal_printf(const char *format, ...) {
#ifdef _ALSP_DEBUG_
    va_list arg;
    char printbuf[PRINT_BUF_SIZE];
    int done;

    va_start(arg, format);
    done = vsnprintf(printbuf, PRINT_BUF_SIZE, format, arg);
    ALOGV("%s", printbuf);
    va_end(arg);

    return done;
#else
    return 0;
#endif
}
int actal_error(const char *format, ...) {
    va_list arg;
    char printbuf[PRINT_BUF_SIZE];
    int done;

    va_start(arg, format);
    done = vsnprintf(printbuf, PRINT_BUF_SIZE, format, arg);
    ALOGE("%s", printbuf);
    va_end(arg);

    return done;
}
int actal_info(const char *format, ...) {
#ifdef _ALSP_RELEASE_    
    return 0;
#else
    va_list arg;
    char printbuf[PRINT_BUF_SIZE];
    int done;

    va_start(arg, format);
    done = vsnprintf(printbuf, PRINT_BUF_SIZE, format, arg);
    ALOGI("%s", printbuf);
    va_end(arg);

    return done;
#endif    
}

void actal_printf_list() {
    if(s_top_p == NULL) {
        ALOGD("list null");
        return;
    }

    struct actal_mem * user_p = s_top_p;
    while (user_p->next != NULL) {
        user_p = user_p->next;
        ALOGD("ptr = %x phy = %x len = %d", (unsigned int)user_p->ptr, user_p->phy_add, user_p->len);
    }
    ALOGD("list end");
}

int actal_get_phyaddr(void *ptr) {
    int align = 0, phy_add = 0;
    struct actal_mem * user_p;

    if (pthread_mutex_lock(&mutex) != 0) {
        ALOGE("get mutex failed");
        return 0;
    }

    user_p = s_top_p->next;
    while (user_p != NULL) {
        align = (int)ptr - (int)user_p->ptr;
        if ((user_p->ptr <= ptr) && (align < user_p->len)) {
            phy_add = user_p->phy_add + align;
            break;
        }
        user_p = user_p->next;
    }
    
    if (pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("free mutex failed");
        return 0;
    }

    if (phy_add == 0) {
        actal_error("error!,phy_add not found. ptr = %X\n", ptr);
    }
    return phy_add;
}

void * actal_get_virtaddr(int phy) {
    int align = 0;
    struct actal_mem * user_p;
    void * virt_add = NULL;

    if (pthread_mutex_lock(&mutex) != 0) {
        ALOGE("get mutex failed");
        return NULL;
    }

    user_p = s_top_p->next;
    while (user_p != NULL) {
        align = phy - user_p->phy_add;
        if ((user_p->phy_add <= phy) && (align < user_p->len)) {
            virt_add = (void*)((int)user_p->ptr + (int)align);
            break;
        }
        user_p = user_p->next;
    }
    
    if (pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("free mutex failed");
        return NULL;
    }

    if (NULL == virt_add) {
        actal_error("phy_add not found. phy = %X\n", phy);
    }
    return virt_add;
}

void *actal_malloc(int size) {
    void *ptr = NULL;

    ptr = (void *)malloc(size);

    actal_printf("malloc : ptr = %#X, size = %#X\n", (int)ptr, size);
    return (void *)((int)ptr);
}

void actal_free(void *ptr) {
    actal_printf("free : add = %#X\n", (int)ptr);
    free((void *)((int)ptr));
}

void check_pid() {
    struct actal_mem * user_p;

    if (pthread_mutex_lock(&mutex) != 0) {
        ALOGE("get mutex failed");
        return ;
    }

    if (s_pid != getpid()) {
        ALOGD("PID changed, reopen ion device");
        ALOGD("parent pid = %d, fd = %d", s_pid, s_fd);
        if (s_top_p != NULL) {
            s_current_p = s_top_p->next;
            while ((user_p = s_current_p) != NULL) {
                s_current_p = user_p->next;
                munmap(user_p->ptr, user_p->len);
                free(user_p);
                user_p = NULL;
            }
            s_top_p->next = NULL;
            s_current_p = s_top_p;
        }
        ion_close(s_fd);
        s_fd = ion_open();
        s_pid = getpid();
        ALOGD("new pid = %d, fd = %d", s_pid, s_fd);
    }
    
    if (pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("free mutex failed");
        return ;
    }
}

void *actal_malloc_wt(int size, int *phy_add) {
    int prot = PROT_READ | PROT_WRITE;
    int map_flags = MAP_SHARED;
    struct ion_handle *handle;
    int map_fd, ret;
    void *ptr;

    if (size <= 0) {
        ALOGE("size must be positive\n");
        return -EINVAL;
    }

    check_pid();

    if (size & ALIGN_MASK) {
        size += (ALIGN_BYTES - (size & ALIGN_MASK));
    }

    struct actal_mem * user_p;

    user_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
    user_p->next = NULL;

    ret = ion_alloc(s_fd, size, 0, 1, 3, &handle);
    if (ret < 0) {
        return NULL;
    }
 
    ret = ion_map(s_fd , handle, size, prot, map_flags, 0, (unsigned char **)&ptr, &map_fd);

    user_p->handle = handle;
    user_p->len = size;
    user_p->fd = s_fd;
    user_p->ptr = ptr;
    user_p->map_fd = map_fd;

    ret = ion_phys(s_fd, handle, (long unsigned int *)phy_add);
    if (ret < 0) {
        actal_error("get phy_addr error!\n");
        return NULL;
    }

    user_p->phy_add = *phy_add;

    if (pthread_mutex_lock(&mutex) != 0) {
        ALOGE("get mutex failed");
        return NULL;
    }

    if (s_top_p == NULL) {
        s_current_p = s_top_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
        s_top_p->fd = 0;
        s_top_p->ptr = NULL;
        s_top_p->map_fd = 0;
        s_top_p->handle = NULL;
        s_top_p->len = 0;
        s_top_p->phy_add = 0;
    }

    s_current_p->next = user_p;
    s_current_p = user_p;

    if (pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("free mutex failed");
        return NULL;
    }

    return (void *)ptr;
}

void actal_free_wt(void *ptr) {
    actal_free_uncache(ptr);
}

void actal_cache_op(void *ptr, int len, int cmd) {
    int ret, align;
    struct actal_mem *user_p, *parent_p;

    if (len <= 0) {
        ALOGE("size must be positive\n");
        return;
    }

    check_pid();

    if (pthread_mutex_lock(&mutex) != 0) {
        ALOGE("get mutex failed");
        return ;
    }

    user_p = s_top_p;
    while (user_p->next != NULL) {
        parent_p = user_p;
        user_p = user_p->next;
        align = (int)ptr - (int)user_p->ptr;
        if ((user_p->ptr <= ptr) && (align < user_p->len)) {
            if (len > (user_p->len - align)) {
                ALOGE("invalid len");
                ALOGE("we got ptr = %x, len = %d, match meory ptr = %x len = %d", (unsigned int)ptr, len, (unsigned int)user_p->ptr, user_p->len);
            } else {
                ret = ion_cache(user_p->fd, user_p->handle, cmd, ptr, align, len);
            }
            goto UNLOCK_MUTEX;
        }
    }
    actal_error("cannot find ptr in list_flush\n");
    UNLOCK_MUTEX:
    if (pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("free mutex failed");
    }
    return ;
}

void actal_cache_flush(void *ptr, int len) {
    actal_cache_op(ptr, len, ION_IOC_CLEAN_CACHES);
}

void actal_cache_env(void *ptr, int len) {
    actal_cache_op(ptr, len, ION_IOC_INV_CACHES);
}

void *actal_malloc_uncache(int size, int *phy_add) {
    int prot = PROT_READ | PROT_WRITE;
    int map_flags = MAP_SHARED;
    struct ion_handle *handle;
    int map_fd, ret;
    void *ptr;
    
    if (size <= 0) {
        ALOGE("size must be positive\n");
        return -EINVAL;
    }

    check_pid();

    if (size & ALIGN_MASK) {
        size += (ALIGN_BYTES - (size & ALIGN_MASK));
    }

    struct actal_mem * user_p;

    user_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
    user_p->next = NULL;
    

    ret = ion_alloc(s_fd, size, 0, 1, 0, &handle);
    if (ret < 0) {
        return NULL;
    }

    ret = ion_map(s_fd , handle, size, prot, map_flags, 0, (unsigned char **)&ptr, &map_fd);

    user_p->handle = handle;
    user_p->len = size;
    user_p->fd = s_fd;
    user_p->ptr = ptr;
    user_p->map_fd = map_fd;

    ret = ion_phys(s_fd, handle, (long unsigned int *)phy_add);
    if (ret < 0) {
        actal_error("get phy_addr error!\n");
        return NULL;
    }

    user_p->phy_add = *phy_add;

    if (pthread_mutex_lock(&mutex) != 0) {
        ALOGE("get mutex failed");
        return NULL;
    }

    if (s_top_p == NULL) {
        s_current_p = s_top_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
        s_top_p->fd = 0;
        s_top_p->ptr = NULL;
        s_top_p->map_fd = 0;
        s_top_p->handle = NULL;
        s_top_p->len = 0;
        s_top_p->phy_add = 0;
    }
    
    s_current_p->next = user_p;
    s_current_p = user_p;
    
    if (pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("free mutex failed");
        return NULL;
    }

    return (void *)ptr;
}

void actal_free_uncache(void *ptr) {
    int ret;
    struct actal_mem *user_p, *parent_p;

    check_pid();

    if (pthread_mutex_lock(&mutex) != 0) {
        ALOGE("get mutex failed");
        return ;
    }

    user_p = s_top_p;

    while (user_p->next != NULL) {
        parent_p = user_p;
        user_p = user_p->next;
        if (user_p->ptr == ptr) {
            ret = ion_free(user_p->fd, user_p->handle);
            munmap(ptr, user_p->len);
            close(user_p->map_fd);
            parent_p->next = user_p->next;
            if (user_p == s_current_p) {
                s_current_p = parent_p;
            }
            free(user_p);
            goto UNLOCK_MUTEX;
        }
    }
    actal_error("cannot find ptr in list\n");
    UNLOCK_MUTEX:
    if (pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("free mutex failed");
    }
    return ;
}

void *actal_malloc_dma(int size, int *phy_add) {
    actal_error("actal_malloc_dma unsupport now, use uncache instead\n");
    return actal_malloc_uncache(size, phy_add);
}

void actal_free_dma(void *ptr) {
    actal_error("actal_malloc_dma unsupport now, use uncache instead\n");
    actal_free_uncache(ptr);
}

int64_t actal_get_ts()  {
    return 0;
}

int actal_get_icinfo() {
    return 0x03;
}

void *actal_memcpy(void *dst, const void *src, int length) {
    return memcpy(dst, src, length);
}

void *actal_memset(void *dst, int v, int length) {
    return memset(dst, v, length);
}

void actal_dump(int *address, int len) {
    int i = 0;
    for (i = 0; i < len; i++) {
        if ((i % 8) == 0) {
            ALOGI("\n%#08X: ", (int) address);
        }
        ALOGI("%08X ", *address);
        address++;
    }

    ALOGI("\n");
}

void actal_sleep_ms(int msec) {
    struct timespec tv;

    if (msec <= 0) {
        return;
    }

    tv.tv_sec = msec / 1000;
    tv.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&tv, NULL);
}

static int __attribute__((constructor)) so_init(void) {
    s_fd = ion_open();
    s_pid = getpid();
    ALOGD("pid = %d", s_pid);
    return 0;
}

static int __attribute__((destructor)) so_exit(void) {
    ion_close(s_fd);
    return 0;
}
