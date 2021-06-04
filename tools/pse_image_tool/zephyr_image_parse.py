#
# Copyright (c) 2019-2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

import sys
import os
import re
import fileinput
import subprocess

ph_re = re.compile('^ +([A-Z]+) +' #Type
                     '(0x[0-9a-f]+) '  # Offset
                     '(0x[0-9a-f]+) '  # VirtAddr
                     '(0x[0-9a-f]+) '  # PhyAddr
                     '(0x[0-9a-f]+) '  # FileSiz
                     '(0x[0-9a-f]+) ')  # MemSiz

map_re = re.compile('^ +([0-9a-f]+) +([0-9a-zA-Z._ ]+)')

memory_re = re.compile('^([_A-Z0-9]+) +'  # Name
                     ' (0x[0-9a-f]+)'  # Origin
                     ' (0x[0-9a-f]+)'  # Length
                     ' ([rwx]+)')   # Attributes

CROSS_COMPILE_TARGET='arm-zephyr-eabi'


if __name__ == "__main__":
    phy_addr = []
    fsize = []
    sections = []
    memory = []
    bin_dic = {}
    bin_start_dic = {}

    print("parsing zephyr image...\n")

    f_map = open("./fragments/zephyr.map");
    for line in f_map:
        match = memory_re.match(line)
        if match:
            print(line.rstrip())
            to_discard = 0
            origin = int(match.group(2), 16)
            length = int(match.group(3), 16)
            if length == 0:
                continue
            for item in memory:
                if item[1] == origin:
                    to_discard = 1
                    break
            if to_discard == 0:
                memory.append([match.group(1), origin, length])

    obj_copy = os.environ.get('ZEPHYR_SDK_INSTALL_DIR') + \
               '/' + CROSS_COMPILE_TARGET + '/bin/' + \
               CROSS_COMPILE_TARGET + '-objcopy'
    f_elf = "./fragments/zephyr.elf"

    readelf = os.environ.get('ZEPHYR_SDK_INSTALL_DIR') + \
               '/' + CROSS_COMPILE_TARGET + '/bin/' + \
               CROSS_COMPILE_TARGET + '-readelf'

    Command = [readelf, '-l', './fragments/zephyr.elf']
    f_seg1 = open("./fragments/segments.txt", 'w+')
    Process = subprocess.Popen (Command, stdout=f_seg1, shell = False)
    Result = Process.communicate()
    f_seg1.close()

    f_seg = open("./fragments/segments.txt")
    for line in f_seg:
        match = ph_re.match(line.rstrip())
        if match:
            fsize.append(int(match.group(5), 16))
            phy_addr.append(int(match.group(4), 16))

        match = map_re.match(line.rstrip())
        if match:
            sections.append(' -j ' + match.group(2).replace(" ", " -j "))

    for n in range(1, len(fsize)):
        if (fsize[n] == 0):
            continue;
        for item in memory:
            if (phy_addr[n] >= item[1]) and \
                   ((phy_addr[n] + fsize[n]) <= (item[1] + item[2])):
                if item[0] == 'L2SRAM':
                    out_bin = 'sram'
                else:
                    out_bin = item[0]
                if out_bin in bin_dic:
                    bin_dic[out_bin] = bin_dic[out_bin] + sections[n]
                else:
                    bin_dic[out_bin] = sections[n]
                    bin_start_dic[out_bin] = phy_addr[n]

    f_seg_start = open("./fragments/seg_start.txt", "w")
    for key in bin_dic.keys():
        f_seg_start.write(key + ' ' + hex(bin_start_dic[key]) + '\n')
        Command = obj_copy + ' -S -Obinary ' + bin_dic[key] + ' ' + f_elf + \
		  ' ./fragments/' + key.lower() + '.bin'
        Process = subprocess.Popen (Command.split(), stdin=subprocess.PIPE,
			stdout = subprocess.PIPE, stderr = subprocess.PIPE,
			shell = False)
        Result = Process.communicate()

    Command="ls -l ./fragments/"
    process = subprocess.Popen(Command.split(), stdin=subprocess.PIPE,
		    stdout=subprocess.PIPE, shell=False)
    output = process.stdout.read()
    print (output.decode())
