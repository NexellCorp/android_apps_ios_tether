#libiconv

LIB_VERSION:=libiconv-android/libiconv

LOCAL_PATH:= $(call my-dir)
LIB_ROOT_REL:= ../../$(LIB_VERSION)
LIB_ROOT_ABS:= $(LOCAL_PATH)/../../$(LIB_VERSION)

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
 -Wno-multichar \
 -D_ANDROID \
 -DLIBDIR="c" \
 -DBUILDING_LIBICONV \
 -DIN_LIBRARY -D__ANDROID__

LOCAL_SRC_FILES := \
 $(LIB_ROOT_REL)/libcharset/lib/localcharset.c \
 $(LIB_ROOT_REL)/lib/iconv.c \
 $(LIB_ROOT_REL)/lib/relocatable.c

LOCAL_C_INCLUDES := \
 $(LIB_ROOT_ABS)/ \
 $(LIB_ROOT_ABS)/../../usbmuxd/out/toolchain/sysroot/usr/include \
 $(LIB_ROOT_ABS)/../../usbmuxd/out/toolchain/include/c++/4.8 \
 $(LIB_ROOT_ABS)/include \
 $(LIB_ROOT_ABS)/lib \
 $(LIB_ROOT_ABS)/libcharset \
 $(LIB_ROOT_ABS)/libcharset/include \
 $(LIB_ROOT_ABS)/srclib

LOCAL_MODULE := libiconv

include $(BUILD_SHARED_LIBRARY)


