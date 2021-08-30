.. _pse_GPIO:

PSE GPIO
###########

Overview
********
A simple PSE GPIO example for intel_pse board.
This application needs connection between two pins. GPIO0 pin 26 is used as output,
and GPIO1 pin 9 is input pin. While output pin is toggled, the input pin will
generate an interrupt and call callback function defined in sample. The
callback function prints out a log to show it entered into callback.

Building and Running
********************
Standard build and run procdure defined for intel_pse target to be
followed.

.. code-block:: GPIO
