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

#ifndef __STREAM_INPUT_H__
#define __STREAM_INPUT_H__

#define DSEEK_SET 0x01
#define DSEEK_END 0x02
#define DSEEK_CUR 0x03

typedef struct stream_input_s {
    int (*read)(struct stream_input_s *stream_input,unsigned char *buf,unsigned int len);
    int (*write)(struct stream_input_s *stream_input,unsigned char *buf,unsigned int len);
    int (*seek)(struct stream_input_s *stream_input,mmm_off_t offset,int original);
    mmm_off_t (*tell)(struct stream_input_s *stream_input);
}stream_input_t;
#endif
