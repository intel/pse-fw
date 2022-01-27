.. _pse_code_relocation_l2sram:

Code Relocation L2SRAM
###########

Overview
********
A simple example for PSE CODE RELOCATION L2SRAM for intel_ehl board.
The sample application will demonstrate how to relocate the application
code text, data and bss section to available memory options, such as
ICCM, DCCM, L2SRAM, and AONRF memory.
This sample app will use L2SRAM for Base FW.
There is a BSS section of the DCCM, L2SRAM, and AONRF that is used
especially for uninitialized variables.
The sample app will print the address of a variable stored in data
section and address of a varible stored in bss section for all the
available memory options.
CMakeLists file controls which memory is used for each of the helper C files
All the instructions, initialized and uninitialized variables in a helper
C file will be stored in the memory option mentioned in CMakeLists.

Building and Running
********************
Standard build and run procedure defined for intel_pse target to be
followed.

Sample Output
=============

.. code-block:: console

    main
    invoke function in L2SRAM..
    function in L2SRAM at address:0x60000b75
    var_in_data_section in L2SRAM at address:0x60027af8
    var_in_bss  in L2SRAM at address:0x600203c0
    invoke function in CCM..
    function in CCM at address:0x00000001
    var_in_data_section in DCCM at address:0x20020000
    var_in_bss in DCCM  at address:0x20020008
    invoke function in split memory..
    function in split mem at address:0x00000039
    var_in_data_section in split mem (DCCM) at address:0x20020004
    var_in_bss in split mem (DCCM) at address:0x2002000c
