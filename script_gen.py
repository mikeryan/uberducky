#!/usr/bin/env python

# Copyright 2019 Mike Ryan
# 
# This file is part of Uberducky and is released under the terms of the
# GPL version 2. Refer to COPYING for more information.

# This tool converts duckyscript into the binary format used internally
# in Uberducky. It outputs a C array to stdout. The script file and the
# name of the array are to be given as command line arguments

import struct
import sys

keywords = set((
    'rem', 'gui', 'string', 'enter',
    'default_delay', 'defaultdelay',
    'delay', 'windows', 'menu', 'app',
    'shift', 'alt', 'control', 'ctrl',
))

canonical = {
    'gui': 'windows',
    'defaultdelay': 'default_delay',
    'app': 'menu',
    'ctrl': 'control',
}

mods = {
    'ctrl': 1,
    'shift': 2,
    'alt': 4,
    'windows': 8,
}

# convert arguments into canonical form
# returns None if there is a problem with the arg
def clean_arg(arg):
    if arg is None or len(arg) == 0:
        return None
    if len(arg) == 1:
        return arg

    arg = arg.lower()
    if arg == 'enter':
        return arg
    if arg == 'space':
        return ' '
    if len(arg) > 1:
        if arg[0] == 'f':
            try:
                num = int(arg[1:])
                if num >= 1 and num <= 12:
                    return arg
            except:
                pass

    return None

# loads a script from a file and returns a list of tuples of (cmd, arg)
def load_script(file):
    parsed_script = []

    with open(file, 'r') as f:
        for line in f:
            line = line.rstrip('\r\n')
            line = line.split(' ', 1)
            cmd = line[0].lower() # normalize commands to lowercase
            arg = None
            if len(line) > 1:
                arg = line[1]
            parsed_script.append((cmd, arg))

    script = []
    default_delay = None

    for cmd, arg in parsed_script:
        if cmd == '':
            continue
        if cmd not in keywords:
            print "Error: unknown command '%s', ignoring" % cmd
            continue
        canon = canonical.get(cmd)
        if canon is not None:
            cmd = canon

        # comments
        if cmd == 'rem':
            pass

        # command followed by argument
        elif cmd in ('windows', 'menu', 'alt', 'control', 'shift'):
            arg = clean_arg(arg)
            if arg is not None:
                script.append((cmd, arg))

        # string of characters
        elif cmd == 'string':
            if len(arg) > 0:
                script.append((cmd, arg))

        # enter key
        elif cmd == 'enter':
            script.append((cmd, None))

        # default delay
        elif cmd == 'default_delay':
            default_delay = int(arg)

        # delay with an argument
        elif cmd == 'delay':
            if arg is None:
                if default_delay is None:
                    raise Exception("no default delay")
                arg = default_delay
            else:
                arg = int(arg)
            script.append((cmd, arg))

    return script

def ducky_to_bin(parsed):
    script = []
    for cmd, arg in s:
        if cmd == 'delay':
            script.append(struct.pack('<BH', 2, arg))
        elif cmd in mods:
            mod = mods[cmd]
            script.append(struct.pack('BBBc', 1, 0, mod, arg))
        elif cmd == 'enter':
            script.append(struct.pack('BBBB', 1, 1, 0, 0))
        elif cmd == 'string':
            l = len(arg)
            script.append(struct.pack('<BH', 3, l))
            script.append(arg)
        else:
            print "not handled %s" % type

    binary_script = ''.join(script)
    l = struct.pack('<H', len(binary_script))

    return ''.join((l, binary_script))

def bin_to_c(script, array_name):
    print '#include <stdint.h>'
    print 'uint8_t %s[%d] = {' % (array_name, len(script))
    print '   ',
    for i in range(0, len(script)):
        print '0x%02x,' % ord(script[i]),
        if i & 7 == 7 and i != len(script) - 1:
            print '\n   ',
    print '\n};'

if __name__ == "__main__":
    try:
        path = sys.argv[1]
        array_name = sys.argv[2]
    except IndexError:
        print "Usage: %s <duckyscript> <array_name>" % sys.argv[0]
        exit(1)

    try:
        s = load_script(path)
        bin = ducky_to_bin(s)
        bin_to_c(bin, array_name)
    except Exception, e:
        print "Problem converting duckyscript: %s" % e
