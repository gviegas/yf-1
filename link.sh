#!/usr/bin/env sh

#
# Makes required sym links.
#

# yf-com
COM_DIR=yf-com
ln -sr test/test.c $COM_DIR/test/test.c
ln -sr test/test.h $COM_DIR/test/test.h

# yf-wsys
WSYS_DIR=yf-wsys
ln -sr test/test.c $WSYS_DIR/test/test.c
ln -sr test/test.h $WSYS_DIR/test/test.h

# yf-core
CORE_DIR=yf-core
ln -sr test/test.c $CORE_DIR/test/test.c
ln -sr test/test.h $CORE_DIR/test/test.h

# yf-ext
EXT_DIR=yf-ext
ln -sr test/test.c $EXT_DIR/test/test.c
ln -sr test/test.h $EXT_DIR/test/test.h
