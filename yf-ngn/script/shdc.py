#!/usr/bin/env python3

#
# YF
# shdc.py
#
# Copyright © 2021 Gustavo C. Viegas.
#

import subprocess

vert_srcs = [
    ('label',    '',        ['-DVPORT_N=1']),
    ('model',    '',        ['-DVPORT_N=1', '-DINST_N=1',  '-DJOINT_N=64']),
    ('model',    'model2',  ['-DVPORT_N=1', '-DINST_N=2',  '-DJOINT_N=64']),
    ('model',    'model4',  ['-DVPORT_N=1', '-DINST_N=4',  '-DJOINT_N=64']),
    ('model',    'model8',  ['-DVPORT_N=1', '-DINST_N=8',  '-DJOINT_N=64']),
    ('model',    'model16', ['-DVPORT_N=1', '-DINST_N=16', '-DJOINT_N=64']),
    ('model',    'model32', ['-DVPORT_N=1', '-DINST_N=32', '-DJOINT_N=64']),
    ('model',    'model64', ['-DVPORT_N=1', '-DINST_N=64', '-DJOINT_N=64']),
    ('particle', '',        ['-DVPORT_N=1']),
    ('quad',     '',        ['-DVPORT_N=1']),
    ('terrain',  '',        ['-DVPORT_N=1'])
]

frag_srcs = [
    ('label',    '', []),
    ('model',    '', ['-DLIGHT_N=16']),
    ('particle', '', []),
    ('quad',     '', []),
    ('terrain',  '', [])
]

src_dir = 'tmp/shd/'
dst_dir = 'bin/'
lang = ''
prefix = 'shd.'
suffix = '.bin'

compiler = 'tmp/shdc'

def compile(src, type, out, extra):
    i = src_dir + src + type + lang
    o = dst_dir + prefix + (src if out == '' else out) + type + suffix
    subprocess.run([compiler, '-V', i, '-o', o] + extra)

def compile_vert():
    for src, out, extra in vert_srcs:
        compile(src, '.vert', out, extra)

def compile_frag():
    for src, out, extra in frag_srcs:
        compile(src, '.frag', out, extra)

if __name__ == '__main__':
    compile_vert()
    compile_frag()
