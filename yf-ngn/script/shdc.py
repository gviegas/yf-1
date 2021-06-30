#!/usr/bin/env python3

#
# YF
# shdc.py
#
# Copyright © 2021 Gustavo C. Viegas.
#

import subprocess

vert_srcs = [
    ('labl', '',      ['-DVPORT_N=1']),
    ('mdl',  '',      ['-DVPORT_N=1', '-DINST_N=1']),
    ('mdl',  'mdl2',  ['-DVPORT_N=1', '-DINST_N=2']),
    ('mdl',  'mdl4',  ['-DVPORT_N=1', '-DINST_N=4']),
    ('mdl',  'mdl8',  ['-DVPORT_N=1', '-DINST_N=8']),
    ('mdl',  'mdl16', ['-DVPORT_N=1', '-DINST_N=16']),
    ('mdl',  'mdl32', ['-DVPORT_N=1', '-DINST_N=32']),
    ('mdl',  'mdl64', ['-DVPORT_N=1', '-DINST_N=64']),
    ('part', '',      ['-DVPORT_N=1']),
    ('quad', '',      ['-DVPORT_N=1']),
    ('terr', '',      ['-DVPORT_N=1'])
]

frag_srcs = [
    ('labl', '', []),
    ('mdl',  '', []),
    ('part', '', []),
    ('quad', '', []),
    ('terr', '', [])
]

src_dir = 'tmp/shd/'
dst_dir = 'bin/'
lang = '.glsl'

compiler = 'tmp/shdc'

def compile(src, type, out, extra):
    subprocess.run([compiler, '-V', src_dir + src + type + lang,
                   '-o', dst_dir + (src if out == '' else out) + type]
                   + extra)

def compile_vert():
    for src, out, extra in vert_srcs:
        compile(src, '.vert', out, extra)

def compile_frag():
    for src, out, extra in frag_srcs:
        compile(src, '.frag', out, extra)

if __name__ == '__main__':
    compile_vert()
    compile_frag()
