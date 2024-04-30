##################################################################################
#
# Copyright (C) 2017 The Android Open Source Project
# Copyright (C) 2020 SeekWave Technology Co.,Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
##################################################################################

LOCAL_PATH := $(call my-dir)

# Make the HAL library
# ============================================================
include $(CLEAR_VARS)

L_CFLAGS := -Wall \
	    -Werror \
	    -Wno-format \
	    -Wno-reorder \
	    -Wno-unused-function \
	    -Wno-unused-parameter \
	    -Wno-unused-private-field \
	    -Wno-unused-variable \
	    -Wno-unused-parameter

L_INCLUDE_DIR += external/libnl/include \
		 $(call include-path-for, libhardware_legacy)/hardware_legacy \
		 external/wpa_supplicant_8/src/drivers \
		 external/boringssl/include \
		 external/boringssl/src/crypto/digest \
		 external/boringssl/src/crypto/evp/

L_LIB_HDR := libutils_headers liblog_headers

L_SRC_FILES := main.cpp \
	       wifi_command.cpp

LOCAL_MODULE := libwifi-hal-skw
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_C_INCLUDES := $(L_INCLUDE_DIR)
LOCAL_SRC_FILES := $(L_SRC_FILES)
LOCAL_HEADER_LIBRARIES := $(L_LIB_HDR)

include $(BUILD_STATIC_LIBRARY)

