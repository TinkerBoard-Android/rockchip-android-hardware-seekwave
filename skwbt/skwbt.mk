	
CUR_PATH := hardware/seekwave/skwbt



PRODUCT_COPY_FILES += $(CUR_PATH)/vendor/etc/bluetooth/skwbt.conf:vendor/etc/bluetooth/skwbt.conf
PRODUCT_COPY_FILES += $(CUR_PATH)/vendor/etc/bluetooth/sv6160.nvbin:vendor/etc/bluetooth/sv6160.nvbin
PRODUCT_COPY_FILES += $(CUR_PATH)/vendor/etc/bluetooth/sv6316.nvbin:vendor/etc/bluetooth/sv6316.nvbin




PRODUCT_PACKAGES += libbt-vendor-seekwave
