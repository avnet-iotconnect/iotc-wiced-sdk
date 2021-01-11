#
# Copyright 2020, Cypress Semiconductor Corporation or a subsidiary of
 # Cypress Semiconductor Corporation. All Rights Reserved.
 # This software, including source code, documentation and related
 # materials ("Software"), is owned by Cypress Semiconductor Corporation
 # or one of its subsidiaries ("Cypress") and is protected by and subject to
 # worldwide patent protection (United States and foreign),
 # United States copyright laws and international treaty provisions.
 # Therefore, you may use this Software only as provided in the license
 # agreement accompanying the software package from which you
 # obtained this Software ("EULA").
 # If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 # non-transferable license to copy, modify, and compile the Software
 # source code solely for use in connection with Cypress's
 # integrated circuit products. Any reproduction, modification, translation,
 # compilation, or representation of this Software except as specified
 # above is prohibited without the express written permission of Cypress.
 #
 # Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 # EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 # WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 # reserves the right to make changes to the Software without notice. Cypress
 # does not assume any liability arising out of the application or use of the
 # Software or any product or circuit described in the Software. Cypress does
 # not authorize its products for use in any products where a malfunction or
 # failure of the Cypress product may reasonably be expected to result in
 # significant property damage, injury or death ("High Risk Product"). By
 # including Cypress's product in a High Risk Product, the manufacturer
 # of such system or application assumes all risk of such use and in doing
 # so agrees to indemnify Cypress against all liability.
 #

# Copyright (C) 2020 Avnet - All Rights Reserved
# Unauthorized copying of this file, via any medium is strictly prohibited
# Proprietary and confidential
# Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
#

NAME := App_IoTConnect_Demo

# -fdiagnostics-color=never use this to avoid garbled output in WICED-Studio on linux
$(NAME)_CFLAGS := -std=c99 -Wall -fdiagnostics-color=never 

$(NAME)_SOURCES := \
	iotconnect_demo.c \
	iotconnect_mqtt_client.c

$(NAME)_COMPONENTS := \
	protocols/MQTT \
	protocols/SNTP \
    libraries/protocols/iotc-c-lib


WIFI_CONFIG_DCT_H := wifi_config_dct.h

$(NAME)_RESOURCES  := apps/iotconnect_demo/rootca.cer \
					  apps/iotconnect_demo/client.cer \
					  apps/iotconnect_demo/privkey.cer

VALID_PLATFORMS := CY8CKIT_062 \
                   NEB1DX_01 \
                   CYW943907AEVAL1F \
                   CYW9MCU7X9N364 \
                   CYW943455EVB_02 \
				   LAIRD_EWB

GLOBAL_DEFINES     += APPLICATION_STACK_SIZE=8196