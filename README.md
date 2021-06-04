Elkhart Lake PSE firmware, services, sample applications and pre-built binaries.
Source code and binaries are distributed under different licenses.
Please read the licenses applicable to them carefully.

PSE Firmware Build Steps:

1. Building default hello world application:
	./build.sh
2. Building any other app
	./build.sh <path_to_app>
3. Cleaninig
	./build.sh clean

Final generated binary has the name PSE_FW.bin which can be used to generate a
signed firmware image for flashing.
