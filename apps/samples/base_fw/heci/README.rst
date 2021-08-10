.. _heci-sample:

PSE HECI
###########

Overview
********

This sample demonstrates how to use the HECI API.

Building and Running
********************

The sample can be built and executed on boards supporting IPC hardware.
1: build HECI sample project.
2: stitch ifwi with the fw generated
3: copy host_client/heci_sample_app.c and Makefile to yocto. compile it with local gcc
4: run the execution binary in yocto

Sample output
=============

.. code-block:: console

   enter
   smpl new heci event 1
   new conn: 0
   smpl new heci event 1
   smpl cmd =  1
   get SMPL_GET_VERSION command
   smpl new heci event 2
   disconnect request conn 0

.. note:: The values shown above might differ.
