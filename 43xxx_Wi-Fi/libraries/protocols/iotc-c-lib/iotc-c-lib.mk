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

NAME := Lib_iotconnect-lib



$(NAME)_SOURCES := \
	src/iotconnect_common.c \
	src/iotconnect_discovery.c \
	src/iotconnect_event.c \
	src/iotconnect_lib.c \
	src/iotconnect_telemetry.c \

$(NAME)_COMPONENTS := utilities/cJSON-iotconnect

#make it visible for the applications which take advantage of this lib
GLOBAL_INCLUDES := include

$(NAME)_CFLAGS += -std=c99 -Wall -Werror -fdiagnostics-color=never