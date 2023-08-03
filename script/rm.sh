#!/usr/bin/env sh

#
# Removes YF lib/header/build files and directories.
#

COM_DIR=yf-com
WSYS_DIR=yf-wsys
CORE_DIR=yf-core
NGN_DIR=yf-ngn

VAR_DIR=~/.local/share/yf
INCLUDE_DIR=$VAR_DIR/include
CACHE_DIR=$VAR_DIR/cache
SHARE_DIR=$VAR_DIR/share
LIB_DIR=$VAR_DIR/lib
BIN_DIR=$VAR_DIR/bin

# yf-ngn
make -C $NGN_DIR -f Lib.mk clean
rmdir -v --ignore-fail-on-non-empty \
			$INCLUDE_DIR/yf/ngn $CACHE_DIR/ngn $SHARE_DIR/ngn

# yf-core
make -C $CORE_DIR -f Lib.mk clean
rmdir -v --ignore-fail-on-non-empty \
			$INCLUDE_DIR/yf/core $CACHE_DIR/core $SHARE_DIR/core

# yf-wsys
make -C $WSYS_DIR -f Lib.mk clean
rmdir -v --ignore-fail-on-non-empty \
			$INCLUDE_DIR/yf/wsys $CACHE_DIR/wsys $SHARE_DIR/wsys

# yf-com
make -C $COM_DIR -f Lib.mk clean
rmdir -v --ignore-fail-on-non-empty \
			$INCLUDE_DIR/yf/com $CACHE_DIR/com $SHARE_DIR/com

# yf
rmdir -v --ignore-fail-on-non-empty \
			$INCLUDE_DIR/yf \
			$INCLUDE_DIR \
			$CACHE_DIR \
			$SHARE_DIR \
			$LIB_DIR \
			$BIN_DIR \
			$VAR_DIR
