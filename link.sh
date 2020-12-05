#!/usr/bin/env sh

#
# Makes required sym links.
#

# yf-com
COM_DIR=yf-com
ln -sr test/test.c $COM_DIR/test/test.c
ln -sr test/test.h $COM_DIR/test/test.h
