/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     csi_core.h
 * @brief    CSI Core Layer Header File
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#ifndef _CORE_H_
#define _CORE_H_

#include <stdint.h>

#if defined(__CK801__) || defined(__E801__)
#include <core_801.h>
#elif defined(__CK802__) || defined(__E802__) || defined(__S802__)
#include <core_802.h>
#elif defined(__E803__) || defined(__S803__)
#include <core_803.h>
#elif defined(__CK803__) || defined(__CK804__) || defined(__E804__) || defined(__E804D__) || defined(__E804F__) || defined (__E804DF__)
#include <core_804.h>
#elif defined(__CK805__) || defined(__I805__) || defined(__I805F__)
#include <core_805.h>
#elif defined(__CK610__)
#include <core_ck610.h>
#elif defined(__CK810__) || defined(__C810__) || defined(__C810V__)
#include <core_810.h>
#elif defined(__CK807__) || defined(__C807__) || defined(__C807F__) || defined(__C807FV__)
#include <core_807.h>
#elif defined(__riscv)
#include <core_rv32.h>
#endif

#ifdef __riscv
#include <csi_rv32_gcc.h>
#else
#include <csi_gcc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* _CORE_H_ */
