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

NAME := Lib_iotc-sdk



$(NAME)_SOURCES := \
	src/iotc_sdk.c \
	src/iotc_wiced_discovery.c \
	src/iotc_wiced_mqtt.c

$(NAME)_COMPONENTS := \
	protocols/iotc-c-lib \
	protocols/MQTT \
	protocols/SNTP \
	protocols/HTTP_client_v2

#make it visible for the applications which take advantage of this lib
GLOBAL_INCLUDES := include

# -fdiagnostics-color=never use this to avoid garbled output in WICED-Studio on linux
$(NAME)_CFLAGS += -std=c99 -Wall -Werror -fdiagnostics-color=never -DWPRINT_ENABLE_LIB_ERROR -DWPRINT_ENABLE_APP_ERROR