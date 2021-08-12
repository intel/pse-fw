.. _pse_pmc_example:

PSE PMC example
###########

Overview
********
A simple PSE PMC service example for intel_pse board.

This application demonstrates how to communicate to the PMC subsystem
securely and issue platform control functionalities such as power down,
restart and wakeup from Sx state.
It also demonstrates how to use the peci short and long format commands.

Building and Running
********************
Standard build and run procdure defined for intel_pse target to be
followed.

Sample Output
=============
.. code-block:: console

	PSE PMC App intel_ehl_crb
	Enter 1 to Shutdown
	Enter 2 to Reset
	Enter 3 to Power up
	Enter 4 to Get Host Temperature
	Enter 5 to Trigger PME assert (hostwakeup)
	Sending PMC short msg to shutdown
	Sx Entry
	Enter 1 to Shutdown
	Enter 2 to Reset
	Enter 3 to Power up
	Enter 4 to Get Host Temperature
	Enter 5 to Trigger PME assert (hostwakeup)
	Sending HW PME_ASSERT to wake up host.
	Sx Exit
	Enter 1 to Shutdown
	Enter 2 to Reset
	Enter 3 to Power up
	Enter 4 to Get Host Temperature
	Enter 5 to Trigger PME assert (hostwakeup)
	Sending PMC short msg to reset
	Enter 1 to Shutdown
	Enter 2 to Reset
	Enter 3 to Power up
	Enter 4 to Get Host Temperature
	Enter 5 to Trigger PME assert (hostwakeup)
	Sx Entry
	Sx Exit
	Enter 1 to Shutdown
	Enter 2 to Reset
	Enter 3 to Power up
	Enter 4 to Get Host Temperature
	Enter 5 to Trigger PME assert (hostwakeup)
	LSB CPU Temp: 128
	MSB CPU Temp: 228
