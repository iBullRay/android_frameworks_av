/*
 *  ion.c
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2011 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <linux/ion.h>

int ion_open();
int ion_close(int fd);
int ion_alloc(int fd, size_t len, size_t align, unsigned int flags,
              struct ion_handle **handle);
int ion_alloc_tiler(int fd, size_t w, size_t h, int fmt, unsigned int flags,
            struct ion_handle **handle, size_t *stride);
int ion_free(int fd, struct ion_handle *handle);
int ion_map(int fd, struct ion_handle *handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd);
int ion_share(int fd, struct ion_handle *handle, int *share_fd);
int ion_import(int fd, int share_fd, struct ion_handle **handle);
int ion_phys(int fd, struct ion_handle *handle, unsigned long *phys);