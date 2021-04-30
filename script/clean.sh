#!/usr/bin/env sh

#
# Removes bin/build files.
#

# yf-com
COM_DIR=yf-com
make -C $COM_DIR -f Devel.mk clean

# yf-wsys
WSYS_DIR=yf-wsys
make -C $WSYS_DIR -f Devel.mk clean

# yf-core
CORE_DIR=yf-core
make -C $CORE_DIR -f Devel.mk clean

# yf-ngn
NGN_DIR=yf-ngn
make -C $NGN_DIR -f Devel.mk clean
