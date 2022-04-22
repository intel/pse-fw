# Intel Elkhart Lake PSE
This repository hosts firmware, services, sample applications and pre-built
binaries for development of software for PSE

## License
Source code and binaries are distributed under different licenses.
Please read the licenses applicable to them carefully.

## Documentation
Detailed documentation with dependencies, build environment setup
and compilation steps have been provided in the docs folder.
Please refer the documents to get started with PSE development.

### Basic PSE Firmware Build Steps:

1. Building default hello world application:
	./build.sh
2. Building any other app
	./build.sh <path_to_app>
3. Cleaninig
	./build.sh clean

Final generated binary has the name PSE_FW.bin which can be used to generate a
signed firmware image for flashing.
