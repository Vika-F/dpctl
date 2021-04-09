//===-- dpctl_sycl_device_manager.h - A manager for sycl devices -*-C++-*- ===//
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
/// This file declares a set of helper functions to query about the available
/// SYCL devices and backends on the system.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "Support/DllExport.h"
#include "Support/ExternC.h"
#include "Support/MemOwnershipAttrs.h"
#include "dpctl_data_types.h"
#include "dpctl_sycl_types.h"
#include "dpctl_vector.h"

DPCTL_C_EXTERN_C_BEGIN

/*! \addtogroup DeviceManager Device class helper functions
 * Helper functions for sycl::device objects that do not directly map to any
 * sycl::device member function.
 * @{
 */

// Declares a set of types abd functions to deal with vectors of
// DPCTLSyclDeviceRef. Refer dpctl_vector_macros.h
DPCTL_DECLARE_VECTOR(Device)

/*!
 * @brief Returns a pointer to a std::vector<sycl::DPCTLSyclDeviceRef>
 * containing the set of ::DPCTLSyclDeviceRef pointers matching the passed in
 * device_identifier bit flag.
 *
 * The device_identifier can be a combination of #DPCTLSyclBackendType and
 * #DPCTLSyclDeviceType bit flags. The function returns all devices that
 * match the specified bit flags. For example,
 *
 *  @code
 *    // Returns all opencl devices
 *    DPCTLDeviceMgr_GetDevices(DPCTLSyclBackendType::DPCTL_OPENCL);
 *
 *    // Returns all opencl gpu devices
 *    DPCTLDeviceMgr_GetDevices(
 *        DPCTLSyclBackendType::DPCTL_OPENCL|DPCTLSyclDeviceType::DPCTL_GPU);
 *
 *    // Returns all gpu devices
 *    DPCTLDeviceMgr_GetDevices(DPCTLSyclDeviceType::DPCTL_GPU);
 *  @endcode
 *
 * @param    device_identifier A bitflag that can be any combination of
 *                             #DPCTLSyclBackendType and #DPCTLSyclDeviceType
 *                             enum values.
 * @return   A #DPCTLDeviceVectorRef containing #DPCTLSyclDeviceRef objects
 * that match the device identifier bit flags.
 */
DPCTL_API
__dpctl_give DPCTLDeviceVectorRef
DPCTLDeviceMgr_GetDevices(int device_identifier);

/*!
 * @brief If the DPCTLSyclDeviceRef argument is a root device, then this
 * function returns a cached default SYCL context for that device.
 *
 * @param    DRef           A pointer to a sycl::device that will be used to
 *                          search an internal map containing a cached "default"
 *                          sycl::context for the device.
 * @return   A DPCTLSyclContextRef associated with the #DPCTLSyclDeviceRef
 * argument passed to the function. If the #DPCTLSyclDeviceRef is not found in
 * the cache, then returns a nullptr.
 * @ingroup DeviceManager
 */
DPCTL_API
DPCTLSyclContextRef
DPCTLDeviceMgr_GetCachedContext(__dpctl_keep const DPCTLSyclDeviceRef DRef);

/*!
 * @brief Get the number of available devices for given backend and device type
 * combination.
 *
 * @param    device_identifier Identifies a device using a combination of
 *                             #DPCTLSyclBackendType and #DPCTLSyclDeviceType
 *                             enum values. The argument can be either one of
 *                             the enum values or a bitwise OR-ed combination.
 * @return   The number of available devices satisfying the condition specified
 * by the device_identifier bit flag.
 */
DPCTL_API
size_t DPCTLDeviceMgr_GetNumDevices(int device_identifier);

/*!
 * @brief Prints out the info::deivice attributes for the device that are
 * currently supported by dpctl.
 *
 * @param    DRef           A #DPCTLSyclDeviceRef opaque pointer.
 */
DPCTL_API
void DPCTLDeviceMgr_PrintDeviceInfo(__dpctl_keep const DPCTLSyclDeviceRef DRef);

/*!
 * @brief Gives the index of the given device in the vector returned get_devices
 * for the platform associated with DRef for the device type of DRef.
 *
 * @param    DRef           A #DPCTLSyclDeviceRef opaque pointer.
 * @ingroup DeviceManager
 */
DPCTL_API
int64_t
DPCTLDeviceMgr_GetRelativeId(__dpctl_keep const DPCTLSyclDeviceRef DRef);

/*! @} */

DPCTL_C_EXTERN_C_END
