ARCHS = armv7 armv7s
TARGET = iphone:clang:latest:5.0

include $(THEOS)/makefiles/common.mk

TOOL_NAME = timefixd
timefixd_FILES = main.c
timefixd_CFLAGS = -Wall -Os

include $(THEOS_MAKE_PATH)/tool.mk
