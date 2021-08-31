.. _pse_ethernet:

PSE Ethernet Test
###########

Overview
********
A simple PSE VNIC example for intel_pse board. The sample application tests
virtual network interface communication between zephyr and linux host.
It pings the IP address of the linux host vnic driver and prints the response.
This sample appilication requires vnic driver on yocto
to be installed for successful execution.

Building and Running
********************

Standard build and run procedure defined for intel_pse target to be
followed.

Sample Output
=============
########## PSE VNIC Test Application #######
uart:~$ [00:00:00.000,003] <inf> vnic: Heci Regn successful
uart:~$ [00:00:00.000,004] <inf> vnic: VNIC MAC: a8:c:d5:a4:37:bf
uart:~$ [00:00:00.000,009] <inf> net_config: Initializing network
uart:~$ [00:00:00.000,009] <inf> net_config: IPv4 address: 192.168.0.1
uart:~$ [00:00:00.000,011] <err> vnic: Connection not ready, tx failure
uart:~$ [00:00:00.177,588] <inf> ping: Sent a ping to 192.168.0.2
uart:~$ [00:00:00.178,372] <inf> ping: Received echo reply from 192.168.0.2 to
192.168.0.1
Starting socket app in 10 seconds..
Starting socket app in 9 seconds..
Starting socket app in 8 seconds..
Starting socket app in 7 seconds..
Starting socket app in 6 seconds..
Starting socket app in 5 seconds..
Starting socket app in 4 seconds..
Starting socket app in 3 seconds..
Starting socket app in 2 seconds..
Starting socket app in 1 seconds..
Sender task started
Sending to dst,count=0
Sent =5 bytes
Receiver task started
RX MSG Len:5
HELLO
.. code-block:: console

