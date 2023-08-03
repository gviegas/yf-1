#!/usr/bin/env sh

#
# Updates local include directory.
#

VAR_DIR=~/.local/share/yf
INCLUDE_DIR=$VAR_DIR/include

cp -uv include/* $INCLUDE_DIR/yf/ngn/
