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
git clone --depth 1 --branch v1.1.0 git://github.com/Avnet/iotc-c-lib.git
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
* The device ID will be generated based on your WiFi MAC address with prefix **wiced-**. 
For example: **wiced-abcdef123456**. You can obtain your WiFi MAC by running the demo on your board.
The MAC address will be printed in the console. Note that the device ID needs to be unique across your CPID, and some 
chips may not use a unique MAC by default. See the WICED SDK instructions on how to configure a custom MAC.  
* Place your device certificate and key in the 43xxx_Wi-Fi/resources/apps/iotconnect_demo/ directory 
in place of client.cer and privkey.cer
* Create a target matching your platform in the right side panel *Make Target" of the C/C++ perspective 
"demo.iotconnect_demo-CYW943907AEVAL1F download run". Replace CYW943907AEVAL1F with your own platform string.
* For Laird EWB: you can use the target *demo.iotconnect_demo-LAIRD_EWB-ThreadX-NetX-SDIO-debug download download_apps run*. 
This target will also make it possible to debug the software as described in the debugging section below.

### Debugging with Laird EWB

(from https://community.cypress.com/thread/32393?start=0&tstart=0)

Ensure that the following items are modified in the default debug configuration before debugging:

* In the Debug Configuration Startup tab, add below instruction into the area below the Halt option:
 
```
add-symbol-file build/eclipse_debug/last_built.elf 0x8000000
``` 

3. In the same Startup tab, clear Run Commands area, default is the phrase "stepi", which should be removed

4. In the Common tab, click the "Launch in background "