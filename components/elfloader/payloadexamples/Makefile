
all: $(patsubst %.c,%-obj.h,$(wildcard *.c)) all.h

debug: all $(patsubst %.c,%-objdump.txt,$(wildcard *.c)) $(patsubst %.c,%-readelf.txt,$(wildcard *.c)) $(patsubst %.c,%.elf,$(wildcard *.c)) 

test-printf-short.o: test-printf-short.c
	xtensa-esp32-elf-gcc -fno-common -Wall  -Werror -o $@ -c $<

test-printf-sections.o: test-printf-sections.c
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -fdata-sections -ffunction-sections -Wall -Werror -o $@ -c $<

test-printf-Os.o: test-printf-Os.c
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -Os -Wall  -Werror -o $@ -c $<

test-printf-O3.o: test-printf-O3.c
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -O3 -Wall  -Werror -o $@ -c $<

test-printf-gdb.o: test-printf-O3.c
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -ggdb -Wall  -Werror -o $@ -c $<

test-printf-stripped.elf: test-printf-stripped.o
	xtensa-esp32-elf-gcc -Wl,-r -nostartfiles -nodefaultlibs -nostdlib -g -Wl,-Tesp32.ld -o $@ $< 
	xtensa-esp32-elf-strip --strip-unneeded $@

%.o: %.c
	xtensa-esp32-elf-gcc -fno-common -mlongcalls -Wall -Werror -o $@ -c $<

%.elf: %.o
	xtensa-esp32-elf-gcc -Wl,-r -nostartfiles -nodefaultlibs -nostdlib -g -Wl,-Tesp32.ld -o $@ $< 

%-objdump.txt: %.elf
	xtensa-esp32-elf-objdump -d -S -s -t -x -r $<  > $@

%-readelf.txt: %.elf
	xtensa-esp32-elf-readelf -a $<  > $@

#%-obj.h: %.elf %-readelf.txt %-objdump.txt
%-obj.h: %.elf
	xxd -i $< > $@

clean:
	rm -f *.o *.elf *.txt *-obj.h all.h

.PHONY: all clean debug

main.elf: main.c
	gcc main.c -o main.elf -lelf

#.PRECIOUS: %-readelf.txt %-objdump.txt %.elf

all.h: $(patsubst %.c,%-obj.h,$(wildcard *.c))
	echo "#ifndef TEST_H" > all.h
	echo "#define TEST_H 1" >> all.h
	ls *.h | awk '{print "#include \"" $$0 "\"" }' >> all.h
	echo "#endif" >> all.h
