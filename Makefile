#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := app-loader
COMPONENTSx= \
	freertos esp32 newlib esptool_py log tcpip_adapter \
	lwip main driver spi_flash nvs_flash vfs ethernet soc heap app_trace \
	bootloader_support xtensa-debug-module wpa_supplicant \
	mbedtls console mdns micro-ecc pthread \
	elfloader
	
include $(IDF_PATH)/make/project.mk
