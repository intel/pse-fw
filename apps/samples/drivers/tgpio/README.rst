.. _pse_tgpio:

PSE TGPIO
#########

Overview
********
Timed GPIO sample application performs tests for adjusting the rate and offset
for timers supported by TGPIO. The application also tests various operating
modes of TGPIO like time based input, event ceiling based input and repeated
output. Test for checking which pins are available as TGPIO pins is also
a part of the application.

Building and Running
********************
Standard build and run procedure defined for intel_pse target to be
followed.
For unlimited output and event ceiling input testing, loopback pins 13 and 16.
For unlimited output and timebased input testing and rate adjustment testing, loopback pins 27 and 28.

Sample Output
*************

.. code-block:: console

	Bind success [TGPIO_0]

	Test for pin availability
	-------------------------
	[pin -  0]  <==Not available
	[pin -  1]  <==Not available
	[pin -  2]  <==Not available
	[pin -  3]  <==Not available
	[pin -  4]  <==Not available
	[pin -  5]  <==Not available
	[pin -  6]  <==Not available
	[pin -  7]  <==Not available
	[pin -  8]  <==Not available
	[pin -  9]  <==Not available
	[pin - 10]  <==Available
	[pin - 11]  <==Available
	[pin - 12]  <==Available
	[pin - 13]  <==Available
	[pin - 14]  <==Available
	[pin - 15]  <==Available
	[pin - 16]  <==Available
	[pin - 17]  <==Available
	[pin - 18]  <==Available
	[pin - 19]  <==Available
	[pin - 20]  <==Available
	[pin - 21]  <==Available
	[pin - 22]  <==Available
	[pin - 23]  <==Available
	[pin - 24]  <==Available
	[pin - 25]  <==Available
	[pin - 26]  <==Available
	[pin - 27]  <==Available
	[pin - 28]  <==Available
	[pin - 29]  <==Available

	Test for time adjustment
	-------------------------
	[TGPIO_0] Timer: 0, Time Now: 100:3020, Adding 99999896 nsecs
	[TGPIO_0] Timer: 0, Adjust Done, Added 99999896 nsecs, Time Now: 100:600660031

	Test for rate adjustment
	-------------------------
	[TGPIO_0] Pin: 28, Timer: 1, Scheduled at:00000101:00000000
	[cb-TGPIO_0] adj_freq: [pin - 27][ts -101: 99976370][ec -  2], running slow with 1860 nsecs
	[TGPIO_0] Adjusting TMT_0 frequency
	[cb-TGPIO_0] adj_freq: [pin - 27][ts -101:199976147][ec -  3], running slow with 223 nsecs
	[cb-TGPIO_0] adj_freq: [pin - 27][ts -101:299976149][ec -  4], running fast with 2 nsecs
	[cb-TGPIO_0] adj_freq: [pin - 27][ts -101:399976147][ec -  5], running slow with 2 nsecs
	[cb-TGPIO_0] adj_freq: [pin - 27][ts -101:499976149][ec -  6], running fast with 2 nsecs
	[cb-TGPIO_0] adj_freq: [pin - 27][ts -101:599976147][ec -  7], running slow with 2 nsecs

	Test for cross timestamp
	-------------------------
	[TGPIO_0] Timer: 0, Local Time: 00000101:629634129, Ref. Time: 00000000:291483229

	Test for unlimited output and timebased input
	-----------------------------------------------
	[TGPIO_0] Pin: 28, Timer: 0, Scheduled at:00000101:01010245
	[TGPIO_0] Pin: 27, Timer: 0, Scheduled at:00000101:01640790

	Test for unlimited output and event ceiling input
	---------------------------------------------------
	[TGPIO_0] Pin: 16, Timer: 0, Scheduled at:00000101:03356255
	[TGPIO_0] Pin: 13, Timer: 0, Scheduled at:00000101:03356255

	[cb-TGPIO_0][pin - 27][ts -101:  1640790][ec -1]
	[cb-TGPIO_0][pin - 13][ts -101:  7356275][ec -5]
	[cb-TGPIO_0][pin - 27][ts -101: 11640790][ec -6]
	[cb-TGPIO_0][pin - 13][ts -101: 12356275][ec -10]
	[cb-TGPIO_0][pin - 13][ts -101: 17356275][ec -15]
	[cb-TGPIO_0][pin - 27][ts -101: 21640790][ec -11]
	[cb-TGPIO_0][pin - 13][ts -101: 22356275][ec -20]
	[cb-TGPIO_0][pin - 13][ts -101: 27356275][ec -25]
