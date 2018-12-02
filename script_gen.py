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
    'delay', 'windows', 'command', 'menu', 'app',
    'shift', 'alt', 'control', 'ctrl',
    'leftarrow', 'left', 'rightarrow', 'right',
    'uparrow', 'up', 'downarrow', 'down',
    'tab', 'esc', 'escape', 'backspace', 'back',
    'space', 'repeat', 'printscreen',
    'f1', 'f2', 'f3', 'f4', 'f5', 'f6',
    'f7', 'f8', 'f9', 'f10', 'f11', 'f12',
))

canonical = {
    'gui': 'windows',
    'command': 'windows',
    'defaultdelay': 'default_delay',
    'app': 'menu',
    'ctrl': 'control',
    'leftarrow': 'left',
    'rightarrow': 'right',
    'uparrow': 'up',
    'downarrow': 'down',
}

mods = {
    'control': 1,
    'shift': 2,
    'alt': 4,
    'windows': 8,
}

special = {
    'enter': 1,
    'tab': 2,
    'esc': 3,
    'back': 4,
}

raw = {
    None: 0x00,
    'menu': 0x76,
    'printscreen': 0x46,
}

# convert arguments into canonical form
# raises Exception if there is a problem with the arg
def clean_arg(arg):
    if arg is None or len(arg) == 0:
        raise Exception('Empty argument')
    if len(arg) == 1:
        return { 'type': 'chr', 'value': arg }

    arg = arg.lower()
    arg = canonical.get(arg, arg)
    if arg == 'space':
        return { 'type': 'chr', 'value': ' ' }
    if arg == 'enter':
        return { 'type': 'special', 'value': 'enter' };
    if arg == 'tab':
        return { 'type': 'special', 'value': 'tab' }
    if arg == 'esc' or arg == 'escape':
        return { 'type': 'special', 'value': 'esc' }
    if arg == 'backspace' or arg == 'back':
        return { 'type': 'special', 'value': 'back' }
    if arg == 'menu':
        return { 'type': 'raw', 'value': 'menu' }
    if arg == 'printscreen':
        return { 'type': 'raw', 'value': 'printscreen' }
    if arg == 'right':
        return { 'type': 'arrow', 'value': 0 }
    if arg == 'left':
        return { 'type': 'arrow', 'value': 1 }
    if arg == 'down':
        return { 'type': 'arrow', 'value': 2 }
    if arg == 'up':
        return { 'type': 'arrow', 'value': 3 }
    if len(arg) > 1:
        # f keys
        if arg[0] == 'f':
            try:
                num = int(arg[1:])
                if num < 1 or num > 12:
                    raise Exception()
            except:
                raise Exception('Invalid f-key "%s"' % arg)

            return { 'type': 'fkey', 'value': num }

    raise Exception('Invalid argument "%s"' % arg)

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

        # multiple modifiers
        if '-' in cmd:
            rawparts = cmd.split('-')
            parts = []
            for p in rawparts:
                if p in keywords:
                    m = canonical.get(p, p)
                    if m not in mods:
                        raise Exception("Invalid mod '%s'" % p)
                    parts.append(m)
            arg = clean_arg(arg)
            arg['mods'] = parts
            script.append(arg)
            continue

        cmd = canonical.get(cmd, cmd)
        if cmd not in keywords:
            raise Exception("Unknown command '%s'" % cmd)

        # modifier key with a normal key argument
        if cmd in mods:
            try:
                arg = clean_arg(arg)
            except:
                # special case: modifier with no argument
                arg = { 'type': 'raw', 'value': None }
            arg['mods'] = [cmd]
            script.append(arg)
            continue

        # comments
        if cmd == 'rem':
            pass

        # string of characters
        elif cmd == 'string':
            if len(arg) > 0:
                script.append({ 'type': 'string', 'value': arg })

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
            script.append({ 'type': 'delay', 'value': arg })

        elif cmd == 'repeat':
            count = 1
            if arg is not None:
                count = int(arg)
            script.append({ 'type': 'repeat', 'value': count })

        else:
            script.append(clean_arg(cmd))

    return script

def ducky_to_bin(parsed):
    script = []
    for cmd in s:
        type, value = cmd['type'], cmd['value']

        mod = 0
        for m in cmd.get('mods', []):
            mod |= mods[m]

        if type == 'delay':
            script.append(struct.pack('<BH', 2, value))
        elif type == 'chr':
            script.append(struct.pack('BBBc', 1, 0, mod, value))
        elif type == 'special':
            script.append(struct.pack('BBBB', 1, special[value], mod, 0))
        elif type == 'fkey':
            script.append(struct.pack('BBBB', 1, 5, mod, 0x3a + value-1))
        elif type == 'arrow':
            script.append(struct.pack('BBBB', 1, 5, mod, 0x4f + value))
        elif type == 'printscreen':
            script.append(struct.pack('BBBB', 1, 5, mod, 0x46))
        elif type == 'raw':
            script.append(struct.pack('BBBB', 1, 5, mod, raw[value]))
        elif type == 'string':
            l = len(value)
            script.append(struct.pack('<BH', 3, l))
            script.append(value)
        elif type == 'repeat':
            script.append(struct.pack('<BH', 4, value))
        else:
            raise Exception("Unhandled command '%s'" % cmd[0])

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
