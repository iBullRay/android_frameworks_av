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

#ifndef _VDE_CORE_H_
#define _VDE_CORE_H_

typedef enum VDE_REG_NO {
    VDE_REG0 = 0,    
    VDE_REG1, VDE_REG2, VDE_REG3, VDE_REG4, VDE_REG5, VDE_REG6, VDE_REG7, VDE_REG8, 
    VDE_REG9, VDE_REG10, VDE_REG11, VDE_REG12, VDE_REG13, VDE_REG14, VDE_REG15, VDE_REG16,
    VDE_REG17, VDE_REG18, VDE_REG19, VDE_REG20, VDE_REG21, VDE_REG22, VDE_REG23, VDE_REG24,
    VDE_REG25, VDE_REG26, VDE_REG27, VDE_REG28, VDE_REG29, VDE_REG30, VDE_REG31, VDE_REG32,
    VDE_REG33, VDE_REG34, VDE_REG35, VDE_REG36, VDE_REG37, VDE_REG38, VDE_REG39, VDE_REG40,    
    VDE_REG41, VDE_REG42, VDE_REG43, VDE_REG44, VDE_REG45, VDE_REG46, VDE_REG_MAX,
}VDE_RegNO_t;   

#define MAX_VDE_REG_NUM (VDE_REG_MAX+1)
#define CODEC_CUSTOMIZE_ADDR (VDE_REG_MAX)
#define CODEC_CUSTOMIZE_VALUE_PERFORMANCE 0x00000001
#define CODEC_CUSTOMIZE_VALUE_LOWPOWER 0x00000002
#define CODEC_CUSTOMIZE_VALUE_DROPFRAME 0x00000004
#define CODEC_CUSTOMIZE_VALUE_MAX 0xffffffff

typedef enum VDE_STATUS {
    VDE_STATUS_IDLE = 0x1,   
    VDE_STATUS_READY_TO_RUN,
    VDE_STATUS_RUNING,
    VDE_STATUS_GOTFRAME,
    VDE_STATUS_JPEG_SLICE_READY = 0x100,
    VDE_STATUS_DIRECTMV_FULL,
    VDE_STATUS_STREAM_EMPTY,
    VDE_STATUS_ASO_DETECTED,
    VDE_STATUS_TIMEOUT = -1,
    VDE_STATUS_STREAM_ERROR = -2,
    VDE_STATUS_BUS_ERROR = -3,
    VDE_STATUS_DEAD = -4,
    VDE_STATUS_UNKNOWN_ERROR = -0x100
}VDE_Status_t;

typedef struct vde_handle {    
    unsigned int (*readReg)(struct vde_handle*, VDE_RegNO_t);
    int (*writeReg)(struct vde_handle*, VDE_RegNO_t, const unsigned int);
    int (*run)(struct vde_handle*);
    int (*query)(struct vde_handle*, VDE_Status_t*);
    int (*query_timeout)(struct vde_handle*, VDE_Status_t*);
    int (*reset)(struct vde_handle*);   
}vde_handle_t;
  
vde_handle_t *vde_getHandle(void);
void vde_freeHandle(struct vde_handle*);
void vde_enable_log(void);
void vde_disable_log(void);
void vde_dump_info(void);


#endif//_VDE_CORE_H_
