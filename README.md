This source has been tested with WICED Studio version 6.6.0

This demo supports only the CA Certificates (x509) based authentication for IoTConnect

### Installation
  
* Download and install the WICED Studio. Note the location of the SDK directory.
* Clone or download the following this repo.
* Clone or download iotc-c-lib tag v1.1.0

* The following git commands can be used to pull all repos:

```bash
git clone --depth 1 --branch v1.7.13 git@github.com:Avnet/iotc-wiced.git
git clone --depth 1 --branch v1.1.0 git://github.com/Avnet/iotc-c-lib.git
```
 
* Copy the contents of the 43xxx_Wi-Fi directory over the same directory inside the WICED SDK directory.
* Copy the contents of the iotc-c-lib directory over the WICED SDK libraries/protocols/iotc-c-lib directory. 
* Alternatively, if you wish to maintain source control,  you can create (symbolic or windows) links of this repo into the corresponding WICED SDK directories.

### Demo Setup

* Launch the WICED IDE.
* Enter your wifi username and password into 43xxx_Wi-Fi/apps/demo/iotconnect_demo/wifi_config.dct.
* Enter your IoTConnect CPID, Enviroment and evice uniqie ID at 43xxx_Wi-Fi/apps/demo/iotconnect_demo/iotconnect_client_config.h.
* Use linux tools provided in tools/ecc-certs at the root of this repository to create certificates for your device. 
Follow the instructions in that directory.
* Place your device certificate and key in the 43xxx_Wi-Fi/resources/apps/iotconnect_demo/ directory.
* Create a target matching your platform in the right side panel *Make Target" of the C/C++ perspective 
"demo.iotconnect_demo-CYW943907AEVAL1F download run". Replace CYW943907AEVAL1F with your own platform string.
* For Laird EWB: you can use the target *demo.iotconnect_demo-LAIRD_EWB-ThreadX-NetX-SDIO-debug download download_apps run*. T
his target will also make it possible to debug the software as described in the section below.

### Debugging with Laird EWB

(from https://community.cypress.com/thread/32393?start=0&tstart=0)

Ensure that the following items are modified in the default debug configuration before debugging:

* In the Debug Configuration Startup tab, add below instruction into the area below the Halt option:
 
```
add-symbol-file build/eclipse_debug/last_built.elf 0x8000000
``` 

3. In the same Startup tab, clear Run Commands area, default is the phrase "stepi", which should be removed

4. In the Common tab, click the "Launch in background "