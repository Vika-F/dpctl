//===----- dpctl_sycl_event_interface.h - C API for sycl::event  -*-C++-*- ===//
//
//                      Data Parallel Control (dpctl)
//
// Copyright 2020-2021 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This header declares a C API to a sub-set of the sycl::event interface.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "Support/DllExport.h"
#include "Support/ExternC.h"
#include "Support/MemOwnershipAttrs.h"
#include "dpctl_data_types.h"
#include "dpctl_sycl_enum_types.h"
#include "dpctl_sycl_types.h"

DPCTL_C_EXTERN_C_BEGIN

/**
 * @defgroup EventInterface Event class C wrapper
 */

/*!
 * @brief A wrapper for ``sycl::event`` contructor to construct a new event.
 *
 * @return   An opaque DPCTLSyclEventRef pointer wrapping a ``sycl::event``.
 * @ingroup EventInterface
 */
DPCTL_API
__dpctl_give DPCTLSyclEventRef DPCTLEvent_Create(void);

/*!
 * @brief C-API wrapper for sycl::event.wait.
 *
 * @param    ERef           An opaque DPCTLSyclEventRef pointer on which to
 *                          wait.
 * @ingroup EventInterface
 */
DPCTL_API
void DPCTLEvent_Wait(__dpctl_keep DPCTLSyclEventRef ERef);

/*!
 * @brief Deletes the DPCTLSyclEventRef after casting it to a ``sycl::event``.
 *
 * @param    ERef           An opaque DPCTLSyclEventRef pointer that would be
 *                          freed.
 * @ingroup EventInterface
 */
DPCTL_API
void DPCTLEvent_Delete(__dpctl_take DPCTLSyclEventRef ERef);

/*!
 * @brief Returns a copy of the DPCTLSyclEventRef object.
 *
 * @param    ERef           DPCTLSyclEventRef object to be copied.
 * @return   A new DPCTLSyclEventRef created by copying the passed in
 * DPCTLSyclEventRef object.
 * @ingroup EventInterface
 */
DPCTL_API
__dpctl_give DPCTLSyclEventRef
DPCTLEvent_Copy(__dpctl_keep const DPCTLSyclEventRef ERef);

/*!
 * @brief  Returns a DPCTLSyclBackendType enum value identifying the SYCL
 * backend associated with the event.
 *
 * @param    ERef           Opaque pointer to a ``sycl::event``
 * @return   A DPCTLSyclBackendType enum value identifying the SYCL backend
 * associated with the event.
 * @ingroup EventInterface
 */
DPCTL_API
DPCTLSyclBackendType DPCTLEvent_GetBackend(__dpctl_keep DPCTLSyclEventRef ERef);

DPCTL_C_EXTERN_C_END