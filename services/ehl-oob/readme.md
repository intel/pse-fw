
### OOB Reference guide

#### Overview

This application is a Elkhart lake reference implementation for an
OOB managability solution to process triggers from the cloud. OOB
is being built as library for Elkhart lake.

However, it can be also be built as a standalone zephyr RTOS app
just the way you build a sample app using open-zephyr

#### Reference Architecture Diagram
coming soon

#### Supported Protocols

MQTT 3.1.1 or 3.1.0 with new mqtt based on
net sock library support

[MQTT](http://mqtt.org/) is a lightweight
publish/subscribe messaging protocol optimized for small sensors and
mobile devices.

See the [MQTT V3.1.1 spec](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html) for more information.

#### Supported Cloud Providers
- Telit previously known as TELIT

#### Requirements

- Linux machine
- SAM_E70 board OR HFPGA for EHL
- Cloud server: This reference version that supports MQTT v3.1.1.
- LAN for testing purposes (Ethernet). Works only in a non-proxy setup.
  Proxy over socks5 is a WIP.

#### Setup
1. Check this self-explanatory guide for the setup for your development or build environment: [setup steps](http://docs.zephyrproject.org/getting_started/getting_started.html)

2. Clone the zephyr-rtos project [opensource-zephyr-rtos](https://github.com/zephyrproject-rtos/zephyr)

3. Create a .zephyrrc file in your home directory with below contents

.. code-block:: console

        export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
        export ZEPHYR_SDK_INSTALL_DIR=<PATH_TO_ZEPHYR_SDK>/zephyr-sdk/
        export ZEPHYR_BASE=<PATH_TO_ZEPHYR_BASE>/zephyr/

**Note:** Replace <PATH_TO_ZEPHYR_SDK> and <PATH_TO_ZEPHYR_BASE> as per your dev environment

4. Source the <PATH_TO_ZEPHYR_BASE>/zephyr-env.sh

#### Build, Flash & Interact

##### Elkhart Lake or "intel_pse" board

###### Build
   OOB is build as a service for the intel_pse board and is structured
   to be built as an library

- Steps to get pre-requisities & codebase [PSE readme](../../../readme.md "PSE setup & build instructions")

- Change the following config options inside these files `pse-dev-code-base/modules/rtos/pse-zephyr/boards/arm/intel_pse/intel_pse_defconfig`
``` bash
CONFIG_OOB_SERVICE=y
CONFIG_OOB_LOGGING=4
```

- Uncomment the below line `pse-dev-code-base/modules/services/ehl-oob/Kconfig`
``` bash
select FPGA_GBE_RGMII_PHY_RX
```

- Add the below lines to `pse-dev-code-base/modules/rtos/pse_zephyr/soc/arm/intel_pse/soc.h`
``` c
#define ETH_DWC_EQOS_0_MAC_ADDR         { 0x00, 0x55, 0x7b, 0xb5, 0x7d, 0xf7 }
#define ETH_DWC_EQOS_1_MAC_ADDR         { 0x00, 0x55, 0x7b, 0xb5, 0x7d, 0xf8 }
```

- Run the below commands from pse-dev-code-base (builds entire pse along with oob)
and creates PSE_FW.bin file under `pse-dev-code-base/tools/script/pse_image_tool/output`

``` bash
./build.sh
```

###### Flash & Interact
- Next steps of stitching the firmware, flashing on FPGA or CRB are beyond the
purview of this readme. Kindly refer to BKC coming from Elkhart lake PSE
to know the details.

##### sam_e70_xplained

###### Build & Flash
   OOB is built an application just like any open-source zephyr app.

- Steps to get [pre-requisities & codebase](https://docs.zephyrproject.org/latest/getting_started/index.html "prerequisites")

- Add below line to the start of this file `pse-dev-code-base/modules/services/ehl-oob/Kconfig`
``` bash
source "Kconfig.zephyr"
```

- Run the below commands

``` bash
export BOARD=sam_e70_xplained
cd pse-dev-code-base/modules/services/ehl-oob
mkdir build && cd build
cmake ..
make flash
```

###### Interact
minicom -D /dev/ttyACM0 -o

#### Sample output

1. OOB performing poweroff command

.. code-block:: console

   [00:00:05.050,010] <inf> OOB_EHL: PSE BIOS startup succeeded...

   [00:00:00.000,157] <inf> i2c_sam_twihs: Device I2C_0 initialized
   [00:00:00.001,334] <inf> eth_sam: MAC: fc:c2:3d:11:98:97
   [00:00:00.001,368] <inf> eth_sam: Queue 0 activated
   [00:00:00.001,368] <inf> eth_sam_phy: Soft Reset of ETH PHY
   [00:00:00.140,004] <inf> eth_sam_phy: PHYID: 0x221561 at addr: 0
   ***** Booting Zephyr OS zephyr-v1.13.0-4168-gb101ddf74e *****
   [00:00:02.040,006] <inf> eth_sam_phy: common abilities: speed 100 Mb, full duplex
   [00:00:05.050,010] <inf> OOB_EHL: PSE BIOS startup succeeded...

   [device_cloud_connect:758] try_to_connect: 0 <OK>
   [publish_event:452] mqtt_publish: 0 <OK>
   [publish_static:536] mqtt_publish: 0 <OK>
   [publish_static:536] mqtt_publish: 0 <OK>
   [publish_static:536] mqtt_publish: 0 <OK>
   [publish_static:536] mqtt_publish: 0 <OK>
   [00:00:06.404,028] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:274] MQTT client connected!

   [00:00:06.483,642] <dbg> OOB_MQTT_CLIENT.publish_event: Publishing event: Elkhart Lake Out-of-band App - Start with message_id: 25246
   [00:00:06.559,085] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:296] PUBACK packet id: 25246

   [00:00:06.559,111] <dbg> OOB_MQTT_CLIENT.publish_static: Publishing static telmetry: key:<CONFIG_ARCH, arm> with packet_id: 29502
   [00:00:06.579,913] <dbg> OOB_MQ[publish_event:452] mqtt_publish: 0 <OK>
   [00:00:06.579,919] <inf> OOB_MTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:342] EVT_PUBLISH packet id: 2165

   [00:00:06.579,919] <inf> OOB_MQTT_CLIENT: MQTT callback received message: �A  on topic: reply
   [00:00:06.579,948] <dbg> OOB_MQTT_CLIENT.publish_static: Publishing static telmetry: key:<CONFIG_SOC_SERIES, Elkhart_Lake> with packet_id: 41945
   [00:00:06.637,348] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:296] PUBACK packet id: 29502

   [00:00:06.637,375] <dbg> OOB_MQTT_CLIENT.publish_static: Publishing static telmetry: key:<CONFIG_BOARD, sam_e70_xplained> with packet_id: 44211
   [00:00:06.655,372] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:296] PUBACK packet id: 41945

   [00:00:06.655,399] <dbg> OOB_MQTT_CLIENT.publish_static: Publishing static telmetry: key:<CONFIG_KERNEL_BIN_NAME, zephyr> with packet_id: 16935
   [00:00:06.706,855] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:342] EVT_PUBLISH packet id: 2165

   [00:00:06.706,860] <inf> OOB_MQTT_CLIENT: MQTT callback received message: �A  on topic: reply
   [00:00:06.706,867] <inf> OOB_EHL: Static Telemetry published...

   [00:00:06.706,888] <dbg> OOB_MQTT_CLIENT.publish_event: Publishing event: Static Telemetry published... with message_id: 37649
   [00:00:06.710,738] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:342] EVT_PUBLISH packet id: 2165

   [00:00:06.710,745] <inf> OOB_MQTT_CLIENT: MQTT callback received message: {"cmd":{"success":true}} on topic: reply
   [00:00:11.480,090] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:296] PUBACK packet id: 44211

   [00:00:11.480,094] <inf> OOB_MQTT_CLIENT: Connected to cloud...

   [00:00:11.480,095] <inf> OOB_MQTT_CLIENT: Listening to commands from cloud...
   [00:00:01.048,308] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:342] EVT_PUBLISH packet id: 2165
   [00:00:01.048,315] <inf> OOB_MQTT_CLIENT: MQTT callback received message: {"sessionId":"5c64648d7663734d6f7a6169","thingKey":"oob-zephyr-same70-karthik"} on topic: notify/mailbox_activity
   [00:00:01.048,331] <inf> OOB_MQTT_CLIENT: Connected to cloud...

   [00:00:01.048,331] <inf> OOB_MQTT_CLIENT: Listening to commands from cloud...

   [publish_api:493] mqtt_publish: 0 <OK>
   [00:00:01.928,205] <dbg> OOB_MQTT_CLIENT.publish_api: Publishing api: {"cmd":{"command":"mailbox.check","params":{"autocomplete": false,"limit":1}}} with message_id: 33148
   [00:00:02.003,898] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:296] PUBACK packet id: 33148

   [00:00:06.048,413] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:342] EVT_PUBLISH packet id: 2166

   [00:00:06.048,423] <inf> OOB_MQTT_CLIENT: MQTT callback received message: {"cmd":{"success":true,"params":{"messages":[{"id":"5c646547b2157c3ee74623ad","thingKey":"oob-zephyr-same70-karthik","from":"karthik.prabhu.vinod@intel.com","command":"method.exec","expires":"2019-02- on topic: reply/mailbox_activity
   [00:00:06.048,445] <inf> OOB_MQTT_CLIENT: Connected to cloud...

   [00:00:06.048,445] <inf> OOB_MQTT_CLIENT: Listening to commands from cloud...

   [publish_event:452] mqtt_publish: 0 <OK>
   [publish_event:452] mqtt_publish: 0 <OK>
   [00:00:08.018,223] <dbg> OOB_MQTT_CLIENT.publish_event: Publishing event: Reboot command Triggered with message_id: 47842
   [00:00:08.094,168] <dbg> OOB_MQTT_CLIENT.mqtt_evt_handler: [mqtt_evt_handler:296] PUBACK packet id: 47842

   [00:00:08.094,173] <inf> OOB_EHL: Reboot command on PMC Firmware returned success

   [00:00:08.094,196] <dbg> OOB_MQTT_CLIENT.publish_event: Publishing event: Reboot command on PMC Firmware returned success with message_id: 51047
