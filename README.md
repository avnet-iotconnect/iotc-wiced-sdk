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
* The device ID will be generated based on your lower case WiFi MAC address with prefix **wiced-**. 
For example: **wiced-a0c1ef123456**. You can obtain your WiFi MAC by running the demo on your board.
The MAC address will be printed in the console. Note that the device ID needs to be unique across your CPID, and some 
chips may not use a unique MAC by default. See the WICED SDK instructions on how to configure a custom MAC.
* Ensure that your device is added to IoTConnect.
* Place your device certificate and key in the 43xxx_Wi-Fi/resources/apps/iotconnect_demo/ directory 
in place of client.cer and privkey.cer
* Create a target matching your platform in the right side panel *Make Target" of the C/C++ perspective 
"demo.iotconnect_demo-CYW943907AEVAL1F download run". Replace CYW943907AEVAL1F with your own platform string.
* For Laird EWB: you can use the target *demo.iotconnect_demo-LAIRD_EWB-ThreadX-NetX-SDIO-debug download download_apps run*. 
This target will also make it possible to debug the software as described in the debugging section below.

### Integrating the SDK Into your own project.

The demo should contain most of the code that you can re-use and add to your application. 

In order to use the SDK in your own project, ensure you follow the steps overlaying your WICED SDK and 
cloning the iotc-c-lib are followed.

Also ensure that your main obtains and obtains the current time before telemetry messages are sent.

In your Makefile include the iotc-sdk library from this repo.

Before the library is initialized, set up the HTTP SEC_TAGs 10702 and 10703 on the modem per nrf_cert_store.h, 
or call:

```c
    err = NrfCertStore_ProvisionApiCerts();
    if (err) {
        printk("Failed to provision API certificates!\n");
    }

    err = NrfCertStore_ProvisionOtaCerts();
    if (err) {
        printk("Failed to provision OTA certificates!\n");
    }
```

Follow the certificates section to set up SEC_TAG 10701 with CA Cert, your device certificate and key.  

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
    if (0 != result) {
        printk("Failed to initialize the SDK\n");
    }

```

You can assign callbacks to NULL or implement on_command, on_ota, and on_connection_status depending on your needs. 

Ether from a task or your main code, call *IotConnectSdk_Loop()* periodically. The function  will 
call the MQTT loop to receive messages. Calling this function more frequently will ensure 
that your commands and OTA mesages are received quicker. Call the function more frequently than CONFIG_MQTT_KEEPALIVE
configured in KConfig.

Set send telemtery messages by calling the iotc-c-lib the library telemetry message functions and send them with 
*IotConnectSdk_SendPacket()*:

```editorconfig
    IOTCL_MESSAGE_HANDLE msg = IOTCL_TelemetryCreate(IotConnectSdk_GetLibConfig());
    IOTCL_TelemetrySetString(msg, "your-name", "your value");
    // etc.
        const char *str = IOTCL_CreateSerializedString(msg, false);
    IOTCL_TelemetryDestroy(msg);
    IotConnectSdk_SendPacket(str);
    IOTCL_DestroySerialized(str);

``` 

Call *IotConnectSdk_Disconnect()* when done.


### Debugging with Laird EWB

(from https://community.cypress.com/thread/32393?start=0&tstart=0)

Ensure that the following items are modified in the default debug configuration before debugging:

* In the Debug Configuration Startup tab, add below instruction into the area below the Halt option:
 
```
add-symbol-file build/eclipse_debug/last_built.elf 0x8000000
``` 

3. In the same Startup tab, clear Run Commands area, default is the phrase "stepi", which should be removed

4. In the Common tab, click the "Launch in background "