#!/usr/bin/env sh

#
# Creates local work tree.
#

VAR_DIR=~/.local/share/yf
INCLUDE_DIR=$VAR_DIR/include
LIB_DIR=$VAR_DIR/lib
BIN=$VAR_DIR/bin
CACHE_DIR=$VAR_DIR/cache
SHARE_DIR=$VAR_DIR/share

mkdir -pv $VAR_DIR $INCLUDE_DIR $LIB_DIR $BIN $CACHE_DIR $SHARE_DIR

# yf-com
mkdir -pv $INCLUDE_DIR/yf/com $CACHE_DIR/com $SHARE_DIR/com
cp -uv yf-com/include/* $INCLUDE_DIR/yf/com/

# yf-wsys
mkdir -pv $INCLUDE_DIR/yf/wsys $CACHE_DIR/wsys $SHARE_DIR/wsys
cp -uv yf-wsys/include/* $INCLUDE_DIR/yf/wsys/

# yf-core
mkdir -pv $INCLUDE_DIR/yf/core $CACHE_DIR/core $SHARE_DIR/core
cp -uv yf-core/include/* $INCLUDE_DIR/yf/core/

# yf-ngn
mkdir -pv $INCLUDE_DIR/yf/ngn $CACHE_DIR/ngn $SHARE_DIR/ngn
cp -uv yf-ngn/include/* $INCLUDE_DIR/yf/ngn/
