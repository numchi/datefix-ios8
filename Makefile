ARCHS = armv7
TARGET = iphone:clang:10.3:8.0

# gzip data.tar for old dpkg on iOS 8 (newer dpkg defaults to zstd, which iOS 8 can't read)
_THEOS_PLATFORM_DPKG_DEB_COMPRESSION = gzip

include $(THEOS)/makefiles/common.mk

TOOL_NAME = timefixd
timefixd_FILES = main.c
timefixd_CFLAGS = -Wall -Os

include $(THEOS_MAKE_PATH)/tool.mk
