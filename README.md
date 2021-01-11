This source has been tested with WICED Studio version 6.6.0

This demo supports only the CA Certificates (x509) based authentication for IoTConnect

### Installation
  
* Download and install the WICED Studio. Note the location of the SDK directory.
* Clone or download the following this repo.
* Clone or fdownload iotc-c-lib tag v1.0.0

* The following git commands can be used to pull all repos:

```bash
git clone --depth 1 --branch v1.7.13 git@github.com:Avnet/iotc-wiced.git
git clone --depth 1 --branch v1.0.0 git://github.com/Avnet/iotc-c-lib.git
```
 
* Copy the contents of the 43xxx_Wi-Fi directory over the same directory inside the WICED SDK directory.
* Copy the contents of the iotc-c-lib directory to WICED SDK libraries/protocols/iotc-c-lib directory. 
* Alternatively, if you wish to maintain source control,  you can create (symbolic or windows) links of this repo into the corresponding WICED SDK directories.

### Demo Setup

* Launch the WICED IDE.
* Enter your wifi username and password into 43xxx_Wi-Fi/apps/demo/iotconnect_demo/wifi_config.dct.
* Use linux tools provided in tools/ecc-certs at the root of this repository to create certificates for your device. Follow the instructions in that directory.
* Place your device certificate and key in the 43xxx_Wi-Fi/resources/apps/iotconnect_demo/ directory.
* Create a target matching your platform in the right side panel *Make Target" of the C/C++ perspective "demo.appliance-CYW943907AEVAL1F download run". Replace CYW943907AEVAL1F with your own platform string. 

### Debugging with LAIRD EWB

(from https://community.cypress.com/thread/32393?start=0&tstart=0)

Ensure that the following items are modified in the default debug configuration before debugging:

* In the Debug Configuration Startup tab, add below instruction into the area below the Halt option:
 
```
add-symbol-file build/eclipse_debug/last_built.elf 0x8000000
``` 

3. In the same Startup tab, clear Run Commands area, default is the phrase "stepi", which should be removed

4. In the Common tab, click the "Launch in background "