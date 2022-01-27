#
# Copyright (c) 2019-2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

from defusedxml.ElementTree import parse
import os
import re
import struct
import sys
import time
import binascii

"""
Header of the manifest structure:
| bup checksum | hearder version | overall_size - man_off |       vector table address (32b)         |
|   reserved   | bup size (32b   | manifest ver (32b)     | misc opt (16b) | iccm_lmt | module count |
"""
HEADER_VER      = 0x10002   # MS16b: Major version | LS16b: Minor version
HEADER_SIZE     = 0x200     # Total size of HEADER, i.e. the offset to the BUP
MAN_OFFSET      = 8 * 4     # = header size
MAN_ENTRY_SZ    = 8 * 4 * 4 # 8 X 32b each manifest
MAN_VER         = 0x10002   # MS16b:-: Major version | LS16b: Minor version

BUP_1ST_STAGE_SIZE      = 0x3000    # 1st stage loaded bringup
PSE_MAN_DEF_COMP_FLAGS  = 0         # default compiler flags
PSE_MAN_DEF_COMP_NAME   = 'XXX'
PSE_MAN_DEF_COMP_VEND   = 'INTC'
PSE_MAN_DEF_DBG_LEVEL   = 1
PSE_MAN_DEF_TRACE_LEVEL = 1
PSE_MAN_DEF_CODE_OFFSET = 0
PSE_MAN_DEF_MODULE_SIZE = 0
PSE_MAN_DEF_LOAD_ADDR   = 0

class PSE_Manifest:
    def __init__(self, comp_flags, comp_name, comp_vendor_name, dbg_level, trace_level, code_offset, module_size, load_address):
        self.comp_name    = comp_name           # component name
        self.vendor_name  = comp_vendor_name    # component vendor
        self.code_offset  = code_offset
        self.module_size  = module_size
        self.load_address = load_address        # address at the target mem
        self.comp_flags   = comp_flags          # component flags
        self.dbg_level    = dbg_level
        self.trace_level  = trace_level

class Module_Info:
    def __init__(self, manifest, raw_bin_path):
        self.manifest = manifest
        self.raw_bin_path = raw_bin_path

class Image_Info:
    def __init__(self, module_info, module_count, output_path):
        self.module_info  = module_info
        self.module_count = module_count
        self.output_path  = output_path
        self.vector_tbl   = 0
        self.iccm_limit   = 0
        self.misc_option  = 0

class Seg_Info:
    def __init__(self, seg_name, seg_start):
        self.seg_name = seg_name
        self.seg_start = seg_start

iccm_re = re.compile('^(ICCM) \s+'  # ICCM
                     ' (0x[0-9a-f]+)'  # Origin
                     ' (0x[0-9a-f]+)'  # Length
                     ' ([rwx]+)\s*')   # Attributes

dccm_re = re.compile('^DCCM \s+'  # DCCM
                     ' (0x[0-9a-f]+)'  # Origin
                     ' (0x[0-9a-f]+)'  # Length
                     ' ([rwx]+)\s*')   # Attributes

vec_re = re.compile('^[\s\t]+0x([0-9a-f])\s+'
                    ' _vector_table')

def str_to_hex(s):
    return int(s.replace("0x", ''), 16)

def is_sub_string(s1, s2):
    tag = False
    if s2.find(s1) != -1:
        tag = True
    return tag

def list_all_member(module):
    print ("----------------------------------------------------------------\n\
        \r\t\tBin path: %s\n\
        \rComp_name:   %s\tVendor_name: %s\n\
        \rComp_flags:  0x%x\tDbg level:    0x%x\n\
        \rTrace_level: 0x%x\tCode_offset:  0x%x\n\
        \rModule_size: 0x%x\tLoad_address: 0x%x"
        %(module.raw_bin_path,\
        module.manifest.comp_name, module.manifest.vendor_name,\
        module.manifest.comp_flags,  module.manifest.dbg_level,\
        module.manifest.trace_level, module.manifest.code_offset,\
        module.manifest.module_size, module.manifest.load_address))

"""
Function: load and parse the config xml file
param: config_file- path of the xml file
       seg_tbl    - Segment infomation table include the actual boot addr
"""
def load_config(config_file, seg_tbl, pse_image_info):
    count = 0

    tree = parse(config_file)
    root = tree.getroot()
    #print (root.tag)
    #print (root[0].text)
    modules = root.find('PSE_MODULES')
    #print (modules.tag)
    #print (modules[0][0].tag, modules[0][0].text)

    """ Parse output """
    output_path = root.find('OUTPUT_PATH')
    pse_image_info.output_path = output_path.text
    misc = root.find('MISC')
    pse_image_info.misc_option = (int)(misc.text)

    """ Parse module count """
    modules_array = []
    for module in modules:
        count += 1
        manifest = PSE_Manifest(PSE_MAN_DEF_COMP_FLAGS, PSE_MAN_DEF_COMP_NAME, PSE_MAN_DEF_COMP_VEND, PSE_MAN_DEF_DBG_LEVEL, PSE_MAN_DEF_TRACE_LEVEL, PSE_MAN_DEF_CODE_OFFSET, PSE_MAN_DEF_MODULE_SIZE, PSE_MAN_DEF_LOAD_ADDR)
        pse_module_info = Module_Info(manifest, '.')
        for bin_path in module.iter('PATH'):
            pse_module_info.raw_bin_path = bin_path.text
        for comp_flags in module.iter('COMP_FLAGS'):
            pse_module_info.manifest.comp_flags = (int)(comp_flags.text)
        for comp_name in module.iter('COMP_NAME'):
            pse_module_info.manifest.comp_name = comp_name.text
        for comp_vendor_name in module.iter('COMP_VENDOR_NAME'):
            pse_module_info.manifest.vendor_name = comp_vendor_name.text
        for dbg_level in module.iter('DBG_LEVEL'):
            pse_module_info.manifest.dbg_level = (int)(dbg_level.text)
        for trace_level in module.iter('TRACE_LEVEL'):
            pse_module_info.manifest.trace_level = (int)(trace_level.text)
        for load_addr in module.iter('LOAD_ADDR'):
            pse_module_info.manifest.load_address = str_to_hex(load_addr.text)
            for seg in seg_tbl:
                if comp_name.text == seg.seg_name:
                    pse_module_info.manifest.load_address = seg.seg_start
                    break
        modules_array.append(pse_module_info)
    pse_image_info.module_count = count
    print ("\r----------------------------------------------------------------\n\
            \rTotal modules found:  %d\n\
            \rOutput path: %s\n\
            \r----------------------------------------------------------------"
        %(count, pse_image_info.output_path))

    pse_image_info.module_info = modules_array
    return pse_image_info

"""
Function:   Generate the final manifest
Parameter:  image_info_xml - parsed from xml file
return:     True  - Bringup.bin is found in fragments sub-dir
            False - Bringup.bin is not found, or xml template corrupted,
                    stitching process stopped
"""
def generate_manifest(image_info):
    bBupFound = False
    cur_off = HEADER_SIZE  ## Bringup is at the beginning
    for i in range(image_info.module_count):
        image_info.module_info[i].manifest.module_size = os.path.getsize(image_info.module_info[i].raw_bin_path)
        print("component: %s\toffset: %X" %(image_info.module_info[i].manifest.comp_name, cur_off))
        image_info.module_info[i].manifest.code_offset = cur_off
        module_sz = image_info.module_info[i].manifest.module_size
        if module_sz & 0xf:
            cur_off = cur_off + 0x10 + (module_sz & 0xfffffff0)
        else:
            cur_off = cur_off + module_sz
        if (image_info.module_info[i].manifest.comp_name == "BRINGUP"):
            if 0 == i:
                bBupFound = True
            else:
                print ("Bringup.bin found, but template xml corrupted!")
    return bBupFound

"""
Function:  Generate the FW image according to the image_info
Parameter: image_info - Parsing result of the XML config file with manifest data
                        to be written into the FW image
           f_output   - file handler of the output bin file
Return:    None
"""
def generate_fw(image_info, f_output):
    print ("\r----------------------------------------------------------------\n\
            \rFirmware stitching start\n\
            \r----------------------------------------------------------------\n\
            \rWriting header information...")

    cur_size = 0
    crc_val  = 0
    if image_info.module_count > 16:
        print ("\r--------------------------------------------------------------------\n\
                \rERROR: Modules to be load is %d\
                \r       The maximum supported modules is 16.\n\
                \rERROR: Firmware stitching stoped!\n\
                \r--------------------------------------------------------------------"
                %image_info.module_count)
        return
    """ written header + manifest structure into byte array """
    for i in range(image_info.module_count):
        if is_sub_string("bringup", image_info.module_info[i].raw_bin_path):
            if 0 != i:
                print ("ERROR: Bad image table! Stitching stopped!")
                return
            bringup_path = image_info.module_info[i].raw_bin_path
            f_bringup_in = open(bringup_path, 'rb')
            n_bringup_sz = os.path.getsize(bringup_path)
            header_data = struct.pack("I", HEADER_VER)      # 2nd DWORD: Header version
            # 3rd DWORD: MS16b overall size
            # 3rd DWORD: LS16b offset of the manifest
            header_data += struct.pack("HH", HEADER_SIZE, MAN_OFFSET)
            # 4th & 5th DWORD: reserved
            # 6th DWORD: bringup size
            # 7th DWORD: version of the manifest
            # 8th DWORD: write actual count of the table
            last_dword = (image_info.module_count -1) | ((image_info.iccm_limit) << 8)\
                         | (image_info.misc_option << 16)
            header_data += struct.pack("5I", image_info.vector_tbl, image_info.misc_option,
                                        n_bringup_sz, MAN_VER, last_dword)
            print("Modules to be loaded: %d\n\
                \rICCM_LIMIT: %d\tMisc Opt:   %X\n"
                %((image_info.module_count - 1), image_info.iccm_limit, image_info.misc_option))
        elif (image_info.module_info[i].manifest.module_size > 0):
            header_data += struct.pack("4s", bytes(image_info.module_info[i].manifest.comp_name, 'ascii'))
            header_data += struct.pack("4s", bytes(image_info.module_info[i].manifest.vendor_name, 'ascii'))
            header_data += struct.pack("6I", image_info.module_info[i].manifest.code_offset, \
                        image_info.module_info[i].manifest.module_size, \
                        image_info.module_info[i].manifest.load_address, \
                        image_info.module_info[i].manifest.comp_flags, \
                        image_info.module_info[i].manifest.dbg_level, \
                        image_info.module_info[i].manifest.trace_level)
        cur_size = cur_size + 32
    else:
        while cur_size < HEADER_SIZE:
            header_data += struct.pack("8I", 0, 0, 0, 0, 0, 0, 0, 0)
            cur_size = cur_size + 32

    ''' calculate CRC32 of the HEADER & write it into file '''
    crc_val = binascii.crc32(header_data, crc_val)

    ''' calculate CRC of the BUP '''
    if n_bringup_sz > BUP_1ST_STAGE_SIZE:
        bup_data = f_bringup_in.read(BUP_1ST_STAGE_SIZE)
    else:
        bup_data = f_bringup_in.read(n_bringup_sz)
    crc_val = binascii.crc32(bup_data, crc_val)

    print("CRC: %X"  %(crc_val))
    f_output.write(struct.pack("I", crc_val))       # 1st DWORD: reserved for CRC32
    f_output.write(header_data)

    """ Generating other sections based on manifest """
    f_output.write(bup_data)
    if n_bringup_sz > BUP_1ST_STAGE_SIZE:
        bup_data = f_bringup_in.read()
        f_output.write(bup_data)
    f_bringup_in.close()

    for i in range(image_info.module_count):
        f_path = image_info.module_info[i].raw_bin_path
        if image_info.module_info[i].manifest.comp_name != "BRINGUP":
            f_output.truncate(image_info.module_info[i].manifest.code_offset)
            f_in = open(image_info.module_info[i].raw_bin_path, 'rb')
            f_output.seek(0, 2)    # Move to the end of the file
            f_output.write(f_in.read(image_info.module_info[i].manifest.module_size))
            f_in.close()
        list_all_member(image_info.module_info[i])
        print ("Stitched: %s to %x" %(f_path, image_info.module_info[i].manifest.code_offset))

    print ("\r----------------------------------------------------------------\n\
            \r-------- Firmware Stitching Done! --------\n\
            \r----------------------------------------------------------------")


"""
Check & get ICCM_LIMIT
Return: True for any error occurred
"""
def map_check():
    pse_image_info = Image_Info(0, 0, 0)
    f_map = open("./fragments/zephyr.map")
    iccm_length = 0
    dccm_length = 0
    for line in f_map:
        match = iccm_re.match(line.rstrip())
        if match:
            iccm_origin = int(match.group(2), 16)
            iccm_length = int(match.group(3), 16)
            print("found ICCM (0x%x,\t0x%x)" % (iccm_origin, iccm_length))
            continue

        match = dccm_re.match(line)
        if match:
            dccm_origin = int(match.group(1), 16)
            dccm_length = int(match.group(2), 16)
            print("found DCCM (0x%x,\t0x%x)" % (dccm_origin, dccm_length))
            continue

        match = re.search('0x([0-9a-f]+)\s+_vector_table', line)
        if match:
            entry_point = int(match.group(1), 16)
            print('found vector table @ 0x%X' %entry_point)
            break

    if (iccm_length & 0xFFFF) > 0:
        iccm_length = iccm_length + 1
    iccm_length  = iccm_length >> 16
    if (dccm_length & 0xFFFF) > 0:
        dccm_length = dccm_length + 1
    dccm_length = dccm_length >> 16
    if iccm_length + dccm_length > 7:
        print ("\r----------------------------------------------------------------\n\
                \rERROR: ICCM size & DCCM size needed exceed the\n\
                \rERROR: Firmware stitching stoped!\n\
                \r----------------------------------------------------------------")
        os._exit(1)
    else:
        pse_image_info.iccm_limit = iccm_length
        pse_image_info.vector_tbl = entry_point
        return pse_image_info

"""
Segment infomation
"""
def get_seg_info():
    seg_info_arr = []
    f_seg = open("./fragments/seg_start.txt")
    for line in f_seg:
        words = line.split()
        seg_info_arr.append(Seg_Info(words[0], int(words[1], 16)))

    f_seg.close
    return seg_info_arr


"""
Module's entry point
"""
def main():
    image_info_xml = map_check()
    seg_table = get_seg_info()
    load_config(config_xml, seg_table, image_info_xml)

    if generate_manifest(image_info_xml):
        f_out = open(image_info_xml.output_path, 'wb')
        generate_fw(image_info_xml, f_out)
        f_out.close
    else:
        print ("\r----------------------------------------------------------------\n\
                \rERROR: bringup.bin is not found or template xml corrupted!\n\
                \rERROR: Firmware stitching stoped!\n\
                \r----------------------------------------------------------------")

if __name__ == "__main__":
    config_xml = "./output/image_tool_setting.xml"
    main()
