#!/usr/bin/env sh

#
# Removes bin/build files.
#

COM_DIR=yf-com
WSYS_DIR=yf-wsys
CORE_DIR=yf-core
NGN_DIR=yf-ngn

# yf-ngn
make -C $NGN_DIR -f Devel.mk clean

# yf-wsys
make -C $WSYS_DIR -f Devel.mk clean

# yf-core
make -C $CORE_DIR -f Devel.mk clean

# yf-com
make -C $COM_DIR -f Devel.mk clean
