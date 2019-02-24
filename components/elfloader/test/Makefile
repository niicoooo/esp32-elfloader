
all: $(patsubst payload-src/%.c,payload-build/%-obj.h,$(wildcard payload-src/*.c)) payload-build

debug: all \
	$(patsubst payload-src/%.c,payload-build/%-objdump.txt,$(wildcard payload-src/*.c)) \
	$(patsubst payload-src/%.c,payload-build/%-readelf.txt,$(wildcard payload-src/*.c))

payload-build:
	mkdir -p payload-build

test-build:
	mkdir -p test-build



########


payload-build/test-printf-shortcall.o: payload-src/test-printf-shortcall.c payload-build
	xtensa-esp32-elf-gcc -fno-common -Wall  -Werror -o $@ -c $<

payload-build/test-printf-sections.o: payload-src/test-printf-sections.c payload-build
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -fdata-sections -ffunction-sections -Wall -Werror -o $@ -c $<

payload-build/test-printf-gdb.o: payload-src/test-printf-gdb.c payload-build
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -ggdb -Wall  -Werror -o $@ -c $<

payload-build/test-printf-stripped.o: payload-src/test-printf-stripped.c payload-build
	xtensa-esp32-elf-gcc -Wl,-r -nostartfiles -nodefaultlibs -nostdlib -g -Wl,-Tesp32.ld -o $@ $< 
	xtensa-esp32-elf-strip --strip-unneeded $@

CCFLAG_test_printf_gdb = -ggdb
CCFLAG_test_printf_O3 = -O3
CCFLAG_test_printf_Os = -Os

ARGIN_test_argvalue = 0x11
ARGOUT_test_argvalue = 0x12
ARGOUT_test_loops1 = 10
ARGOUT_test_loops2 = 0
ARGOUT_test_return_bss = 0x12345678
ARGOUT_test_return_bss_volatile = 0x12345678
ARGOUT_test_return_rwdata = 0x12345678
ARGOUT_test_return_rwdata_volatile = 0x12345678
ARGOUT_test_return_value = 0x12345678




########


payload-build/%.o: payload-src/%.c payload-build
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -Wall -Werror $(CCFLAG_$(subst -,_,$(patsubst payload-src/%.c,%,$<))) -o $@ -c $<

payload-build/%.elf: payload-build/%.o
	xtensa-esp32-elf-gcc -Wl,-r -nostartfiles -nodefaultlibs -nostdlib -g -Wl,-Tesp32.ld -o $@ $<

payload-build/%-objdump.txt: payload-build/%.elf
	xtensa-esp32-elf-objdump -d -S -s -t -x -r $<  > $@

payload-build/%-readelf.txt: payload-build/%.elf
	xtensa-esp32-elf-readelf -a $<  > $@

payload-build/%-obj.h: payload-build/%.elf test-build
	xxd -i $< > $@

	$(eval ARG0 := $(subst -,_,$(patsubst payload-build/%-obj.h,%,$@)))
	$(eval ARG1 := $(patsubst payload-build/%-obj.h,%,$@))
	$(eval ARG2 := $(if $(ARGIN_$(subst -,_,$(patsubst payload-build/%-obj.h,%,$@))),$(ARGIN_$(subst -,_,$(patsubst payload-build/%-obj.h,%,$@))),0x00))
	$(eval ARG3 := $(if $(ARGOUT_$(subst -,_,$(patsubst payload-build/%-obj.h,%,$@))),$(ARGOUT_$(subst -,_,$(patsubst payload-build/%-obj.h,%,$@))),0x00))
	
	@echo
	@echo var ARGIN_$(subst -,_,$(patsubst payload-build/%-obj.h,%,$@))
	@echo ARG1: $(ARG1)
	@echo ARG2: $(ARG2)
	@echo ARG3: $(ARG3)
	@echo

	cat template.c.in | \
		sed -e 's/{{0}}/$(ARG0)/' | \
		sed -e 's/{{1}}/$(ARG1)/' | \
		sed -e 's/{{2}}/$(ARG2)/' | \
		sed -e 's/{{3}}/$(ARG3)/' \
		> $(patsubst payload-build/%.h,test-build/%.c,$@)

clean:
	rm -r payload-build test-build

.PHONY: all debug
