#!/bin/bash
#
# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#
python_ver=3.6.9
cmake_ver=3.22.1
dtc_ver=1.4.7
ninja_ver=1.8.2

Help()
{
   # Display Help
   echo "This script will install all the dependencies for building Zephyr based project. It will prompt user for Zephyr SDK installation path right after launch"
   echo "Syntax: ./install_sdk_env.sh [h]"
   echo "options:"
   echo "h     Print this Help."
   echo
}

################################################################################
# Process the input options.                             					   #
################################################################################
# Get the options
while getopts ":h" option; do
   case $option in
      h) # display Help
         Help
         exit;;
	  \?)# incorrect option
         echo "Error: Invalid option"
		 echo "Use -h for help"
		 exit;;
   esac
done

################################################################################
# Main program                                                                 #
################################################################################
current_dir=$PWD
read -p "Enter Zephyr SDK installation director(Press Enter for default path(/opt/zephyr-sdk2): " installdir

#Updating system and depencies for Zephyr build
yes | sudo apt update
yes | sudo apt upgrade
yes | sudo apt-get install --no-install-recommends git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev

pip3 install defusedxml

if [[ -f "$HOME/.zephyrrc" ]];
then
	mv ~/.zephyrrc ~/.zephyrrc.bak
	touch ~/.zephyrrc
	chmod +x ~/.zephyrrc
else 
	touch ~/.zephyrrc
	chmod +x ~/.zephyrrc
fi

pip3 install --user -U west
echo "export PATH=~/.local/bin:"$PATH"" >> ~/.zephyrrc

mkdir -p ~/bin
cd ~/bin

#Fetching and installing Zephyr SDK
if [[ -f "$HOME/bin/zephyr-sdk-0.13.1-linux-x86_64-setup.run" ]];
then
	echo "Zephyr SDK installer already downloaded in target directory"
elif wget "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.13.1/zephyr-sdk-0.13.1-linux-x86_64-setup.run";
then
	echo "Downloaded Zephyr SDK"
else
	echo -e "\e[1;31m Zephyr SDK Installer not found \e[0m"
fi 

if [[ -f "$HOME/bin/zephyr-sdk-0.13.1-linux-x86_64-setup.run" ]];
then	
	chmod +x zephyr-sdk-0.13.1-linux-x86_64-setup.run
	if [ -z "$installdir" ]
	then
		installdir=/opt/zephyr-sdk2
		echo "SDK install dir: $installdir"
	else
		echo "SDK install dir: $installdir"
	fi
	
	if [ -d "$installdir" ]
	then
	sudo rm -rf $installdir
	fi
	
	sudo ./zephyr-sdk-0.13.1-linux-x86_64-setup.run -- -d $installdir
	echo "export ZEPHYR_TOOLCHAIN_VARIANT=zephyr" >> ~/.zephyrrc
	echo "export ZEPHYR_SDK_INSTALL_DIR=$installdir" >> ~/.zephyrrc
#	echo "export PATH=$installdir/arm-zephyr-eabi/bin/:\$PATH" >> ~/.zephyrrc
fi


#Checking Python version 
var="$(python3 --version 2>&1)"
if [ ${var//[!0-9]} \> ${python_ver//[!0-9]} ] 
then
	echo "Python version higher than $python_ver already installed"
elif [[ "$var" =~ "$python_ver" ]]
then
	echo "Python $python_ver already installed"
	echo "alias python=python3" >> ~/.zephyrrc
else
	echo "Installing Python $python_ver"
	sudo apt-get install python3-pip
	echo "alias python=python3" >> ~/.zephyrrc
fi

#Checking Cmake version 
var="$(cmake --version 2>&1)"
if [ ${var//[!0-9]} \> ${cmake_ver//[!0-9]} ]
then
	echo "Cmake higher than $cmake_ver already installed"
elif [[ $var =~ "$cmake_ver" ]] 
then
	echo "Cmake $cmake_ver already installed"
else
	echo "Installing Cmake $cmake_ver"
	mkdir -p $HOME/bin/cmake && cd $HOME/bin/cmake
	if [[ -f "$HOME/bin/cmake/cmake-3.22.1.tar.gz" ]];
	then
		echo "Cmake installer already downloaded in target directory"
	elif wget "https://github.com/Kitware/CMake/releases/download/v3.22.1/cmake-3.22.1.tar.gz"; 
	then
		echo "Downloaded Cmake installer"
    else 
		echo -e "\e[1;31m Cmake $cmake_ver Installer not found \e[0m"
	fi
	
	if [[ -f "$HOME/bin/cmake/cmake-3.22.1.tar.gz" ]];
	then
		sudo rm -rf cmake-3.22.1
		sudo tar -zxvf cmake-3.22.1.tar.gz
		cd cmake-3.22.1
		yes | sudo apt-get install build-essential
		yes | sudo apt-get install libssl-dev
		sudo ./bootstrap
		sudo make
		sudo make install
		#echo "export PATH=\$PATH:$PWD/bin" >> $HOME/.zephyrrc
	fi
fi

#Checking device-tree-compiler (dtc) version 
var="$(dtc --version 2>&1)"
if [ ${var//[!0-9]} \> ${dtc_ver//[!0-9]} ]
then
	echo "device-tree-compiler (dtc) higher than $dtc_ver already installed"
elif [[ $var =~ "$dtc_ver" ]] 
then
	echo "device-tree-compiler (dtc) $dtc_ver already installed"
else
	echo "Installing device-tree-compiler (dtc) $dtc_ver"
	cd ~/bin
	if [[ -f "$HOME/bin/device-tree-compiler_1.4.7.orig.tar.xz" ]];
	then
		echo "device-tree-compiler (dtc) installer already downloaded in target directory"
	elif wget "https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/device-tree-compiler/1.4.7-3/device-tree-compiler_1.4.7.orig.tar.xz";
	then
		echo "Downloaded device-tree-compiler (dtc) installer"
	else 
	echo -e "\e[1;31m device-tree-compiler (dtc) $dtc_ver Installer not found \e[0m"
	fi
   
   if [[ -f "$HOME/bin/device-tree-compiler_1.4.7.orig.tar.xz" ]];
   then
		sudo rm -rf dtc-1.4.7
		sudo tar xvf device-tree-compiler_1.4.7.orig.tar.xz
		yes | sudo apt-get install flex bison swig python-dev
		cd dtc-1.4.7 && sudo make
		echo "export PATH=$PWD:\$PATH" >> ~/.zephyrrc 
    fi
fi

#Installing additional Python packages required by Zephyr
cd $current_dir
cd ../../../zephyr
pip3 install --user -r scripts/requirements.txt

source ~/.zephyrrc

#Checking Python version 
ver_py=0
var="$(python3 --version 2>&1)"
if [ ${var//[!0-9]} \> ${python_ver//[!0-9]} ] 
then
	ver_py=1
elif [[ "$var" =~ "$python_ver" ]]
then
	ver_py=1
else
	echo -e "\e[1;31m Install Python version $python_ver or higher manually \e[0m"
fi

#Checking Cmake version 
ver_cmk=0
var="$(cmake --version 2>&1)"
if [ ${var//[!0-9]} \> ${cmake_ver//[!0-9]} ]
then
	ver_cmk=1
elif [[ $var =~ "$cmake_ver" ]] 
then
	ver_cmk=1
else
	echo -e "\e[1;31m Install Cmake version $cmake_ver or higher manually \e[0m"
fi

#Checking device-tree-compiler (dtc) version 
ver_dtc=0
var="$(dtc --version 2>&1)"
if [ ${var//[!0-9]} \> ${dtc_ver//[!0-9]} ]
then
	ver_dtc=1
elif [[ $var =~ "$dtc_ver" ]] 
then
	ver_dtc=1
else
	echo -e "\e[1;31m Install device-tree-compiler (dtc) version $dtc_ver or higher manually \e[0m"
fi

if [[ $ver_py -eq 1 && $ver_cmk -eq 1 && $ver_dtc -eq 1 ]]
then
	python3 --version
	cmake --version
	dtc --version
	echo -e "\e[1;32m \n Finished Installation \e[0m"
else
	echo -e "\e[1;31m \n Check above errors \e[0m"
fi
