#! /bin/bash
ws_dir=$(pwd)
SERVICES=$(pwd)/services
pse_image_tools_dir=$ws_dir/tools/pse_image_tool
bootloader_dir=$ws_dir/../modules/bootloader

clean() {
	if [ -d "build" ]; then
		rm -rf build
	fi
	if [ -d $pse_image_tools_dir/fragments ]; then
		rm -rf $pse_image_tools_dir/fragments
	fi
	if [ -d $pse_image_tools_dir/output ]; then
		rm -rf $pse_image_tools_dir/output
	fi
	pushd $bootloader_dir > /dev/null
	cd aon_task;
	if [ -f aon_task.bin ]; then
		make clean;
	fi
	cd ../bringup
	if [ -f bringup.bin ]; then
		make clean;
	fi

	popd > /dev/null

}

if [ "$1" == "clean" ]; then
	echo "Cleaning.."
	clean
	echo "done"
	exit 0
fi


if [ "$1" == "" ]
then
	app=$ws_dir/apps/capabilities/pse_base
	echo "Building pse_base app"
else
	app=$1
fi
clean
west build -b ehl_pse_crb $app -DZEPHYR_EXTRA_MODULES=$SERVICES

if [ ! -d "build" ]; then
  echo "Build Failed"
  exit 0
fi
if [ $(test) -n "$(find build -name zephyr.elf)" ]; then
	echo "build passed"
	ZEPHYR_ELF=$(find build -name zephyr.elf)
	ZEPHYR_MAP=$(find build -name zephyr.map)
	mkdir $pse_image_tools_dir/fragments
	cp $ZEPHYR_ELF $ZEPHYR_MAP $pse_image_tools_dir/fragments
	if [ "$2" == "CUSTOMER" ]; then
		echo "CUSTOMER BUILD"
		if [ -f "tools/bin/aon_task.bin" ]; then
			echo "found aon_task bin"
			cp tools/bin/aon_task.bin $pse_image_tools_dir/fragments
		fi
		if [ -f "tools/bin/bringup.bin" ]; then
			echo "found bringup bin"
			cp tools/bin/bringup.bin $pse_image_tools_dir/fragments
		fi
	else
		pushd $bootloader_dir > /dev/null
		cd aon_task; make SILICON;
		cp aon_task.bin $pse_image_tools_dir/fragments
		cd ../bringup; make SILICON
		cp bringup.bin $pse_image_tools_dir/fragments
		popd > /dev/null
	fi
	pushd $pse_image_tools_dir > /dev/null
	python3 zephyr_image_parse.py
	make
	if [ -f output/PSE_FW.bin ]; then
		echo -e "Stiched Firmware binary at:\n" $(pwd)/output/PSE_FW.bin
	fi
	popd > /dev/null
else
	echo "build failed"
fi
