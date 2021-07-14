.. _pse_user_mode_app_and_services:

PSE User Mode App and Services Example
###########

Overview
********
A simple examples for PSE USER MODE APPLICATION AND SERVICE for intel_pse_crb
board.

User mode apps and services -

Simple app with user mode framework
App which need to access some kernel objects and have multiple sub task
App which need to access some kernel objects and device drivers
Service which will execute immediately after post kernel init but
before Apps main

Building and Running
********************
Standard build and run procdure defined for intel_pse_crb target to be
followed.

Sample Output
=============

.. code-block:: console

	Service 4 Main
	Service 4 Main return
	Service 5 Main
	Service 5 Main return
	App 6, g_cnt:1
	App 7 Main
	App 7 Main return
	App 8 Main
	Main Service 4 Task 0
	Main Service 4 Task 0 done
	Main Service 5 Task 0
	Main Service 5 Task 0 done
	Main APP 7 Task 1 started
	Sem given 1
	Sem given 2
	Sem given 3
	Sem given 4
	Sem given 5
	Sem given 6
	Main APP 7 Task 1 done
	Main Service 4 Task 1
	hello this message from task 0 Service 0
	Main Service 4 Task 1 done
	Main Service 5 Task 1
	hello this message from task 0 Service 1
	Main Service 5 Task 1 done
	Main APP 7 Task 2 started
	Sem taken 6
	Sem taken 5
	Sem taken 4
	Sem taken 3
	Sem taken 2
	Sem taken 1
	Main APP 7 Task 2 done
