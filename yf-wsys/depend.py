#!/usr/bin/env python3

#
# YF
# depend.py
#
# Copyright © 2020 Gustavo C. Viegas.
#

import pty
import sys
import os

src_dir = 'etc/'
inc_dir = 'etc/'
wayland_proto = '/usr/share/wayland/wayland.xml'
wayland_code = src_dir + 'wayland-protocol-private.c'
wayland_hdr = inc_dir + 'wayland-client-protocol.h'
xdg_shell_proto = '/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml'
xdg_shell_code = src_dir + 'xdg-shell-protocol-private.c'
xdg_shell_hdr = inc_dir + 'xdg-shell-client-protocol.h'

def gen_code(input, output):
    pty.spawn(['wayland-scanner', '-c', 'private-code', input, output])

def gen_hdr(input, output):
    pty.spawn(['wayland-scanner', '-c', 'client-header', input, output])

def gen_wayland(code_only=True):
    gen_code(wayland_proto, wayland_code)
    if not code_only:
        gen_hdr(wayland_proto, wayland_hdr)

def gen_xdg_shell(code_only=True):
    gen_code(xdg_shell_proto, xdg_shell_code)
    if not code_only:
        gen_hdr(xdg_shell_proto, xdg_shell_hdr)

def clean():
    files = [wayland_code, wayland_hdr, xdg_shell_code, xdg_shell_hdr]
    for file in files:
        try:
            os.remove(file)
        except:
            pass

if __name__ == "__main__":
    if '--clean' in sys.argv[1:]:
        clean()
    else:
        code_only = False if '--header' in sys.argv[1:] else True
        os.makedirs(src_dir, exist_ok=True)
        os.makedirs(inc_dir, exist_ok=True)
        gen_wayland(code_only)
        gen_xdg_shell(code_only)
