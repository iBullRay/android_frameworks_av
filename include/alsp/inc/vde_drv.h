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

#if 0
#include "vpx_drv.h"

#define VDE_QUERY VPX_QUERY
#define VDE_ENABLE_CLK VPX_ENABLE_CLK
#define VDE_DISABLE_CLK VPX_DISABLE_CLK
#define VDE_RUN VPX_RUN
#define VDE_REGISTER_BASE 0xb0310000

#else

#ifdef __cplusplus
extern "C" {
#endif

#define VDE_MAGIC_NUMBER 'v'
#define VDE_QUERY _IOWR(VDE_MAGIC_NUMBER, 0xf0, unsigned long)
#define VDE_ENABLE_CLK _IOWR(VDE_MAGIC_NUMBER, 0xf1, unsigned long)
#define VDE_DISABLE_CLK _IOWR(VDE_MAGIC_NUMBER, 0xf2, unsigned long)
#define VDE_RUN _IOWR(VDE_MAGIC_NUMBER, 0xf3, unsigned long)
#define VDE_DUMP _IOWR(VDE_MAGIC_NUMBER, 0xf4,unsigned long)
#define VDE_SET_FREQ _IOWR(VDE_MAGIC_NUMBER, 0xf5, unsigned long)
#define VDE_GET_FREQ _IOWR(VDE_MAGIC_NUMBER, 0xf6,unsigned long)
#define VDE_SET_MULTI _IOWR(VDE_MAGIC_NUMBER, 0xf7,unsigned long)

#define VDE_FREQ_DEFAULT 360
#define VDE_FREQ_D1 180
#define VDE_FREQ_720P 240
#define VDE_FREQ_1080P 60
#define VDE_FREQ_MULTI 480
#define VDE_FREQ_4Kx2K 540

#ifdef __cplusplus
}
#endif

#endif
