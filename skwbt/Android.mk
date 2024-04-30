
LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)



BDROID_DIR := $(TOP_DIR)system/bt
ifeq ($(PLATFORM_VERSION),13)
BDROID_DIR := $(TOP_DIR)packages/modules/Bluetooth/system
else ifeq ($(PLATFORM_VERSION),14)
BDROID_DIR := $(TOP_DIR)packages/modules/Bluetooth/system
endif


LOCAL_SRC_FILES := \
        src/bt_vendor_skw.c \
        src/scom_vendor.c \
        src/skw_log.c \
	src/skw_gen_addr.c \
	src/skw_btsnoop.c 


LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
	$(BDROID_DIR)/hci/include 

LOCAL_C_INCLUDES += $(bdroid_C_INCLUDES)
LOCAL_CFLAGS += $(bdroid_CFLAGS)




LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libutils \
        liblog

LOCAL_MODULE := libbt-vendor-seekwave
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := seekwave
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

#PRODUCT_PACKAGES += libbt-vendor-seekwave
