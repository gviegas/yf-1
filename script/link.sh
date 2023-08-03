#!/usr/bin/env sh

#
# Makes required sym links.
#

COM_DIR=yf-com
WSYS_DIR=yf-wsys
CORE_DIR=yf-core
NGN_DIR=yf-ngn

# yf-com
ln -srv test/test.c $COM_DIR/test/test.c
ln -srv test/test.h $COM_DIR/test/test.h

# yf-wsys
ln -srv test/test.c $WSYS_DIR/test/test.c
ln -srv test/test.h $WSYS_DIR/test/test.h

# yf-core
ln -srv test/test.c $CORE_DIR/test/test.c
ln -srv test/test.h $CORE_DIR/test/test.h

# yf-ngn
ln -srv test/test.c $NGN_DIR/test/test.c
ln -srv test/test.h $NGN_DIR/test/test.h
