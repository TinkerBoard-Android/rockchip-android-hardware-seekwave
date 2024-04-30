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

ifeq ($(BOARD_WLAN_DEVICE), seekwave)

WPA_SRC_DIR = external/wpa_supplicant_8

SKW_LIB_INC = $(WPA_SRC_DIR)/src \
		$(WPA_SRC_DIR)/src/common \
		$(WPA_SRC_DIR)/src/drivers \
		$(WPA_SRC_DIR)/src/l2_packet \
		$(WPA_SRC_DIR)/src/utils \
		$(WPA_SRC_DIR)/src/wps \
		$(WPA_SRC_DIR)/wpa_supplicant

SKW_LIB_INC += external/libnl/include
SKW_LIB_SRC = driver_cmd_nl80211.c

include $(WPA_SRC_DIR)/wpa_supplicant/android.config
L_CFLAGS += -Wall -Werror -Wno-unused-parameter -Wno-macro-redefined

ifeq ($(TARGET_ARCH),arm)
# To force sizeof(enum) = 4
L_CFLAGS += -mabi=aapcs-linux
endif

ifdef CONFIG_ANDROID_LOG
L_CFLAGS += -DCONFIG_ANDROID_LOG
endif

ifdef CONFIG_P2P
L_CFLAGS += -DCONFIG_P2P
endif

########################

include $(CLEAR_VARS)
LOCAL_MODULE := lib_driver_cmd_skw
LOCAL_LICENSE_KINDS := SPDX-license-identifier-BSD
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := $(SKW_LIB_SRC)
LOCAL_C_INCLUDES := $(SKW_LIB_INC)
LOCAL_VENDOR_MODULE := true
include $(BUILD_STATIC_LIBRARY)

########################

endif
