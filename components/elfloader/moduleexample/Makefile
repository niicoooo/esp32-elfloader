#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := app-module

module: module.elf module-readelf.txt module-objdump.txt 

module.elf: component-main-build
	xtensa-esp32-elf-gcc -Wl,-r -nostartfiles -nodefaultlibs -nostdlib -g -o $@ -Lbuild/main -lmain -Wl,-e,local_main -Wl,-Tesp32.ld
	xtensa-esp32-elf-strip --strip-unneeded $@
	
%-objdump.txt: %.elf
	xtensa-esp32-elf-objdump -d -S -s -t -x -r $<  > $@

%-readelf.txt: %.elf
	xtensa-esp32-elf-readelf -a $<  > $@

include $(IDF_PATH)/make/project.mk

clean:
	rm -f module.elf module-readelf.txt module-objdump.txt  rm -rf build
