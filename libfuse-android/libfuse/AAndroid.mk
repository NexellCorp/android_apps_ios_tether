# Copyright (C) 2012 Seth Huang<seth.hg@gmail.com>
#

# Build libfuse

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS	:= -DANDROID -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 \
	-D__MULTI_THREAD
LOCAL_MODULE    := libfuse
LOCAL_STATIC_LIBRARIES := libfuse
LOCAL_SRC_FILES := cuse_lowlevel.c fuse.c fuse_kern_chan.c fuse_loop.c \
	fuse_loop_mt.c fuse_lowlevel.c fuse_mt.c fuse_opt.c fuse_session.c \
	fuse_signals.c helper.c mount.c mount_util.c ulockmgr.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(TARGET_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

# Build fusermount

include $(CLEAR_VARS)

LOCAL_MODULE := fusermount

LOCAL_SRC_FILES := fusermount.c

LOCAL_CFLAGS :=  -DANDROID -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 \
	-D__MULTI_THREAD -Wall -DUSE_AUTH -D_XOPEN_SOURCE=500 -D_ISOC99_SOURCE \
	-DIGNORE_MTAB

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(TARGET_C_INCLUDES)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_LIBRARIES := libfuse

include $(BUILD_EXECUTABLE)

# Build httpfs2 (as an example)

include $(CLEAR_VARS)

LOCAL_MODULE := httpfs

LOCAL_SRC_FILES := httpfs2.c

LOCAL_CFLAGS :=  -DANDROID -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 \
	-D__MULTI_THREAD -Wall -DUSE_AUTH -D_XOPEN_SOURCE=500 -D_ISOC99_SOURCE

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(TARGET_C_INCLUDES)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_LIBRARIES := libfuse

include $(BUILD_EXECUTABLE)

