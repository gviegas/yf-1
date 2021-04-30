#!/usr/bin/env sh

#
# Uninstalls YF.
#

# yf-ngn
# TODO

# yf-core
CORE_DIR=yf-core
make -C $CORE_DIR -f Lib.mk uninstall

# yf-wsys
WSYS_DIR=yf-wsys
make -C $WSYS_DIR -f Lib.mk uninstall

# yf-com
COM_DIR=yf-com
make -C $COM_DIR -f Lib.mk uninstall

# yf
INSTALL_DIR=/usr/local
YF_DIR=$INSTALL_DIR/include/yf
rmdir --ignore-fail-on-non-empty $YF_DIR
