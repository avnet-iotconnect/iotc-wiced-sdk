This repository contains IoTConnect WICED SDK and samples intended for use with Avnet's IoTConnect platform.

The sample shows how to make use of the IoTConnect SDK to connect your devices to IoTConnect.

This source has been tested with WICED Studio version 6.6.0

This sdk and the demo supports only the CA Certificates (x509) based authentication for IoTConnect

### Installation
  
* Download and install the WICED Studio. Note the location of the SDK directory.
* Clone or download the this repo.
* Clone or download iotc-c-lib tag v1.1.0

* The following git commands can be used to pull the repos:

```bash
git clone --depth 1 git@github.com:Avnet/iotc-wiced-sdk.git
git clone --depth 1 --branch v2.0.0 git://github.com/Avnet/iotc-c-lib.git
```
 
* Copy the contents of the 43xxx_Wi-Fi directory over the same directory inside the WICED SDK root directory.
* Copy the contents of the iotc-c-lib directory over the WICED SDK libraries/protocols/iotc-c-lib directory. 
* Alternatively, if you wish to maintain source control, you can create (symbolic or windows) links of this 
repo into the corresponding WICED SDK directories, but don't use git --depth argument.

### Demo Setup

* Launch the WICED IDE.
* Enter your WiFi username and password into 43xxx_Wi-Fi/apps/demo/iotconnect_demo/wifi_config.dct.
* Enter your IoTConnect CPID and Enviroment at 43xxx_Wi-Fi/apps/demo/iotconnect_demo/iotconnect_client_config.h.
* Use linux tools provided in tools/ecc-certs at the root of iotc-c-lib repo in order to to create certificates
 for your device. Follow the instructions in that directory.
* The device ID will be generated based on your lower case WiFi MAC address with prefix **wiced-**. 
For example: **wiced-a0c1ef123456**. You can obtain your WiFi MAC by running the demo on your board.
The MAC address will be printed in the console. Note that the device ID needs to be unique across your CPID, and some 
chips may not use a unique MAC by default. See the WICED SDK instructions on how to configure a custom MAC.
* Ensure that your device is added to IoTConnect.
* Place your device certificate and key in the 43xxx_Wi-Fi/resources/apps/iotconnect_demo/ directory 
in place of client.cer and privkey.cer
* Create a target matching your platform in the right side panel *Make Target" of the C/C++ perspective 
"demo.iotconnect_demo-CYW943907AEVAL1F download run". Replace CYW943907AEVAL1F with your own platform string.
* For Laird EWB: you need to use target *demo.iotconnect_demo-LAIRD_EWB-ThreadX-NetX-SDIO-debug download download_apps run*. 
This target will also make it possible to debug the software as described in the debugging section below.
* For CY8CKIT-062-WiFi-BT  you need to use target *demo.iotconnect_demo-CY8CKIT_062-debug download download_apps run*
This target will also make it possible to debug the software as described in the debugging section below.

### Integrating the SDK Into your own project.

The demo should contain most of the code that you can re-use and add to your application. 

In order to use the SDK in your own project, ensure you follow the steps overlaying your WICED SDK and 
cloning the iotc-c-lib are followed.

In your Makefile include the iotc-sdk library from this repo.

Ensure that your main obtains the current time before telemetry messages are sent. time() function implementation 
is also required. Your application should also load the device certificate and key and provide it to the library.

In your application code, initialize the SDK:

```editorconfig
    IOTCONNECT_CLIENT_CONFIG *config = IotConnectSdk_InitAndGetConfig();
    config->cpid = "Your CPID";
    config->duid = "Your Cevice Unique ID";
    config->env = "Your Environment";
    config->cmd_cb = on_command;
    config->ota_cb = on_ota;
    config->status_cb = on_connection_status;

    int result = IotConnectSdk_Init();
```
 
You can assign callbacks to NULL or implement on_command, on_ota, and on_connection_status depending on your needs. 

Set send telemetry messages by calling the iotc-c-lib the library telemetry message functions and send them with 
*IotConnectSdk_SendPacket()*. The packet ID obtained can be used for cross-referencing purposes in the 
MQTT_PUBLISHED event of the SDK:

```editorconfig
    IOTCL_MESSAGE_HANDLE msg = IOTCL_TelemetryCreate(IotConnectSdk_GetLibConfig());
    
    IOTCL_TelemetrySetString(msg, "your-name", "your value");

    const char *str = IOTCL_CreateSerializedString(msg, false);
    IOTCL_TelemetryDestroy(msg);
    wiced_mqtt_msgid_t pktId = IotConnectSdk_SendPacket(str);
    WPRINT_APP_INFO(("Sending packet ID %u: %s\n", pktId, str));
    IOTCL_DestroySerialized(str);
``` 

Call *IotConnectSdk_Disconnect()* when done.

### Debugging with Laird EWB

(from https://community.cypress.com/thread/32393?start=0&tstart=0)

* The build target needs to be `demo.iotconnect_demo-CY8CKIT_062-debug download download_apps run`

Ensure that the following items are modified in the default debug configuration for your OS 
(eg. 43xxx_Wi-Fi_Debug_Linux64) before debugging:

* In the Debug Configuration Startup tab, add below instruction into the area below the Halt option:

```
add-symbol-file build/eclipse_debug/last_built.elf 0x8000000
``` 

* In the same Startup tab, clear Run Commands area, default is the phrase "stepi", which should be removed
* In the Common tab, click the "Launch in background"

### Debugging with CY8CKIT-062-WiFi-BT (PSoC 6 WiFi-BT Pioneer Kit)

(from https://community.cypress.com/t5/WICED-Studio-Wi-Fi-Combo/Debugger-failing-to-launch-quot-error-erasing-flash-with/m-p/28810)

Ensure that the following items are modified in the default debug configuration for your OS 
(eg. 43xxx_Wi-Fi_Debug_Linux64) before debugging:

* In the Debug Configuration Startup tab modify the following items:
  * Uncheck the "Load image" checkmark.
  * Check Set Breakpoint checkamrk and enter: `application_start`
  * Check the Resume checkmark.
  * CLear the Run Commands area, default is the phrase "stepi", which should be removed
  * Add these instructions into the area below the Halt option:
```
monitor targets psoc6.cpu.cm4
monitor psoc6 reset_halt sysresetreq
monitor sleep 50
```

* In the Common tab, click the "Launch in background"