This is a Yocto tool to dump PSE tracing data

usages:
- Enable FW tracing in the prj.conf of your app
	add those configuration options:
		CONFIG_TRACING=y
		CONFIG_TRACING_CTF=y
		CONFIG_TRACING_BACKEND_PSE_SHMEM=y
	set "CONFIG_THREAD_NAME=y" to show thread name strings in tracing. Not
	MUST, but highly suggest to have.
	The default trace buffer size is 64K. To change it, set
	"CONFIG_TRACING_CTF_BOTTOM_SHMEM_SIZE=xxx", xxx must be (N * 4096).

- Rebuild your zephyr app, burn and run on board

- add "iomem=relaxed" into linux boot option while booting

- scp or copy trace_dump.c onto Yocto /data/

- in Yocto shell:
	echo on >/sys/bus/pci/devices/0000\:00\:1d.0/power/control #disable RTD3
	cd /data/
	gcc -o ptdump ./trace_dump.c
	./ptdump
	CTRL + C (stop dumping, there will be a new file /data/channel0_0)

- Parsing on Ubuntu, there are two tools to use, either of them.
	- babeltrace2 (https://babeltrace.org/)
		 apt-add-repository ppa:lttng/ppa
		 apt-get update
		 apt-get install babeltrace2
	- TraceCompass (https://www.eclipse.org/tracecompass/)
		download in the web

	create an empty directory
	copy ./modules/rtos/pse_zephyr/subsys/debug/tracing/ctf/tsdl/metadata
		into it
	scp or copy /data/channel0_0 of Yocto into it

	in the directory, run "babeltrace2 convert --clock-cycles ./".
	or open TraceCompass, click "File->Open Trace", browse into the
		directory, double click file channel0_0
