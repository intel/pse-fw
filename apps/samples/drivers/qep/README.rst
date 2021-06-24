.. _pse_qep:

PSE QEP
###########

Overview
********
A simple PSE QEP example for intel_pse board.
The application demonstarates the event capturing and qudarature decoding mode
for qep. The application requries qep module to be interfaced with an optical
quadrature encoder.

This application can be built into modes:

* single thread
* multi threading

Building and Running
********************
Standard build and run procdure defined for intel_pse target to be
followed.

Sample Output
=============
Testing Edge Capture Mode

Waiting for 5 Edges
Capture done for given count.
Edge cap done , len =5
Timestamp 0: 2928186996
Timestamp 1: 2936668216
Timestamp 2: 2937311706
Timestamp 3: 2937744256
Timestamp 4: 2938228056
Passed

Testing QEP Decode Mode
Disabled Watchdog timeout
Position counter reset to counting up.
Disabled Direction change
Position Count  = 25
Clockwise
Decode Stopped

.. code-block:: console
