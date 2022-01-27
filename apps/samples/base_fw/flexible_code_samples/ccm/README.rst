.. _pse_code_relocation_ccm:

Code Relocation CCM
###########

Overview
********
A simple example for PSE CODE RELOCATION CCM for intel_ehl board.
The sample application will demonstrate how to relocate the application
code text, data and bss section to available memory options, such as
ICCM, DCCM, L2SRAM, and AONRF memory.
This sample app will use ICCM and DCCM for Base FW.
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
    function in L2SRAM at address:0x60000001
    var_in_data_section in L2SRAM at address:0x60000100
    var_in_bss  in L2SRAM at address:0x60000108
    invoke function in CCM..
    function in CCM at address:0x0000098d
    var_in_data_section in DCCM at address:0x20027cf8
    var_in_bss in DCCM  at address:0x200205c0
    invoke function in split memory..
    function in split mem at address:0x60000039
    var_in_data_section in split mem (L2SRAM) at address:0x60000104
    var_in_bss in split mem (L2SRAM) at address:0x6000010c
