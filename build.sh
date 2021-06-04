#! /bin/bash
ws_dir=$(pwd)
pse_image_tools_dir=$ws_dir/tools/pse_image_tool

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
}

if [ "$1" == "clean" ]; then
	echo "Cleaning"
	clean
	exit 0
fi

if [ "$1" == "" ]
then
	app=$ws_dir/../zephyr/samples/hello_world
	echo "Building Hello World"
else
	app=$1
fi
clean
west build -b ehl_pse_crb $app

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
	if [ -f "tools/bin/aon_task.bin" ]; then
		echo "found aon_task bin"
		cp tools/bin/aon_task.bin $pse_image_tools_dir/fragments
	fi
	if [ -f "tools/bin/bringup.bin" ]; then
		echo "found bringup bin"
		cp tools/bin/bringup.bin $pse_image_tools_dir/fragments
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

