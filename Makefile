#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := app-loader
#EXTRA_COMPONENT_DIRS += $(shell pwd)/../components/elfloader


example: main/payload.h all

example-payload/payload.elf:
	echo Building payload...
	env -i IDF_PATH=$(IDF_PATH) PATH=$(PATH) make -C example-payload
	echo Building payload done

main/payload.h: example-payload/payload.elf
	xxd -i $< > $@
	

include $(IDF_PATH)/make/project.mk
