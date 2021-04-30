#!/usr/bin/env sh

#
# Installs YF.
#

# yf
INSTALL_DIR=/usr/local
YF_DIR=$INSTALL_DIR/include/yf
mkdir -p $YF_DIR

# yf-com
COM_DIR=yf-com
make -C $COM_DIR -f Lib.mk install

# yf-wsys
WSYS_DIR=yf-wsys
make -C $WSYS_DIR -f Lib.mk install

# yf-core
CORE_DIR=yf-core
make -C $CORE_DIR -f Lib.mk install

# yf-ngn
# TODO
