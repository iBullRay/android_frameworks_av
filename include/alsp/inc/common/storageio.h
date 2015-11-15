/*
 * Copyright (C) 2008 Actions-Semi, Inc.
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

#ifndef __STORAGEIO_H__
#define __STORAGEIO_H__

typedef struct storage_io_s {
    int (*read)(void *buf, int size, int count, struct storage_io_s *io);
    int (*write)(void *buf, int size, int count, struct storage_io_s *io);
    int (*seek)(struct storage_io_s *io, mmm_off_t offset, int whence);
    mmm_off_t (*tell)(struct storage_io_s *io);
}storage_io_t;

#endif // __STORAGEIO_H__
