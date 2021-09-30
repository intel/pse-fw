.. _pse_pwm_counter_test:

PSE Test for PWM and counters
###########

Overview
********
This is a sample PSE PWM application for the intel_pse board.
This test verifies the APIs for PWM IP in following modes -
1. PWM mode (Using pwm driver)
2. Counter mode (Using counter driver)

Two separate drivers are used by this test application.
Both drivers are for the same IP but for different functionalities.

To validate the output in PWM mode, you need to connect the oscilloscope
to pin3 on PWM_0 and pin7 on PWM_1.

Building and Running
********************
Standard build procedure.

Sample Output
=============

PWM_0 - Bind Success
PWM_1 - Bind Success
COUNTER_11 - Bind Success
COUNTER_4 - Bind Success
Running rate: 20000000
configured pin 3 (Period: 20, pulse: 5)
Stop pulses on pin 3, No o/p produced
Start pulses on pin 3, o/p restored
[call_back] Timer 11 expired
[call_back] Timer 11 expired
[call_back] Timer 11 expired
[call_back] Timer 11 expired
[call_back] Timer 11 expired
[call_back] Timer 4 expired
[call_back] Timer 11 expired



