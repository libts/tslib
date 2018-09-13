LOCAL_PATH:= $(call my-dir)
LOCAL_MODULE_TAGS := optional

include $(CLEAR_VARS)

TSLIB_PLUGINDIR := /system/lib/ts/plugins

LOCAL_SRC_FILES := \
        src/ts_attach.c \
        src/ts_close.c \
        src/ts_config.c \
        src/ts_error.c \
        src/ts_fd.c \
        src/ts_get_eventpath.c \
        src/ts_load_module.c \
        src/ts_open.c \
        src/ts_option.c \
        src/ts_parse_vars.c \
        src/ts_read.c \
        src/ts_read_raw.c \
	src/ts_setup.c \
	src/ts_version.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/
LOCAL_CFLAGS := -DTS_CONF=\"/system/etc/ts.conf\"
LOCAL_CFLAGS += -DPLUGIN_DIR=\"/system/lib/ts/plugins\"

LOCAL_SHARED_LIBRARIES := libdl

LOCAL_MODULE := libts
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)


# plugin: input-raw
include $(CLEAR_VARS)

LOCAL_SRC_FILES := plugins/input-raw.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/input
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: pthres
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/pthres.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/pthres
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: linear
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/linear.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/
LOCAL_CFLAGS += -DTS_POINTERCAL=\"/system/etc/pointercal\"

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/linear
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: dejitter
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/dejitter.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/dejitter
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: skip
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/skip.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/skip
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: debounce
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/debounce.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/debounce
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: median
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/median.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/median
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: iir
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/iir.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/iir
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: lowpass
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/lowpass.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/lowpass
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# plugin: invert
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := plugins/invert.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts/plugins/invert
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# ts_calibrate
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  tests/testutils.c \
        tests/fbutils-linux.c \
        tests/font_8x8.c \
        tests/font_8x16.c \
        tests/ts_calibrate_common.c \
        tests/ts_calibrate.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/
LOCAL_CFLAGS := -DTS_POINTERCAL=\"/system/etc/pointercal\"

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts_calibrate
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


# ts_test_mt
include $(CLEAR_VARS)

LOCAL_SRC_FILES := tests/testutils.c \
        tests/fbutils-linux.c \
        tests/font_8x8.c \
        tests/font_8x16.c \
        tests/ts_test_mt.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts_test_mt
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


# ts_print
include $(CLEAR_VARS)

LOCAL_SRC_FILES := tests/ts_print.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts_print
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


# ts_print_mt
include $(CLEAR_VARS)

LOCAL_SRC_FILES := tests/testutils.c \
	tests/ts_print_mt.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts_print_mt
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


# ts_print_raw
include $(CLEAR_VARS)

LOCAL_SRC_FILES := tests/ts_print_raw.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts_print_raw
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


# ts_uinput
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  tools/ts_uinput.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/

LOCAL_SHARED_LIBRARIES := libdl \
                        libts

LOCAL_MODULE := ts_uinput
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
