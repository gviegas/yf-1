#!/usr/bin/env sh

#
# Builds YF tests.
#

(script/var.sh; script/link.sh)

COM_DIR=yf-com
WSYS_DIR=yf-wsys
CORE_DIR=yf-core
NGN_DIR=yf-ngn

# yf-com
make -C $COM_DIR -f Devel.mk -j 4

# yf-wsys
make -C $WSYS_DIR -f Devel.mk -j 4

# yf-core
make -C $CORE_DIR -f Devel.mk -j 4

# yf-ngn
make -C $NGN_DIR -f Devel.mk -j 4
