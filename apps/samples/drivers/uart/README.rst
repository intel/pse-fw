.. _pse_uart:

PSE Uart
###########

Overview
********
A simple PSE UART  example for intel ehl_pse_crb board.
Tests UART for polled and interrupt based operation.
Application assumes that UART_2 console is being used for printk and
executes tests on UART_1.
This application can be built into modes:

* single thread
* multi threading

Building and Running
********************
Standard build and run procedure defined for ehl_pse_crb target to be
followed.

Sample Output
=============
UART_2 Console Output

PSE UART Test Application
Testing Device UART_1

Buffered polled Mode on UART_1
Buffered Polled Write passed
 
Buffered polled read , input 5 characters on test port
Data entered by User : 12345
Buffered Polled Read passed

Asynchronous write on UART_1
Write Data : len =31  status =0
Asynchronous Write passed
 
Asynchronous read on UART_1
Waiting for 10 characters on UART_1
Data Entered by User : 1234567890
Read Data : len =10  status =0
Asynchronous Read passed

Exiting UART App.

UART_1 Console Output

12345
12345
Test Async Output for UART.
1231234567

.. code-block:: console
