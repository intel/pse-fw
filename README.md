# Intel Elkhart Lake PSE
This is the primary repository that hosts firmware, services, sample
applications and pre-built binaries for development of software for PSE.
Elkhart Lake PSE relies on code from multple repositories fetched using
west tool to compile the applications.
Please refer to the documentation in docs folder for more details.

## License
Source code and binaries are distributed under different licenses.
Please read the licenses applicable to them carefully.

## Documentation
Detailed documentation with dependencies, build environment setup
and compilation steps have been provided in the docs folder.
Please refer the documents to get started with PSE development.


### Steps for getting PSE code(requires west & related tools)
1. west init -m https://github.com/intel/pse-fw.git --mr main
2. west update

# Steps for checking out a specific release tag
1. west init -m https://github.com/intel/pse-fw.git --mr main
2. west update
3. ./ehl_pse-fw/scripts/tag_checkout.sh <TAG_NAME>

### Basic PSE Firmware Build Steps:

1. Building default hello world application:
	./build.sh
2. Building any other app
	./build.sh <path_to_app>
3. Cleaninig
	./build.sh clean

Final generated binary has the name PSE_FW.bin which can be used to generate a
signed firmware image for flashing.
