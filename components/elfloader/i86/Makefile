SRC=main.c ../loader.c ../unaligned.c
TARGET=elfloader

CROSS=arm-none-eabi-
CC=gcc
AS=gcc
LD=gcc

CFLAGS=-O0 -ggdb3 \
	-ffunction-sections -fdata-sections \
	-I. -Wall -Werror

LDFLAGS=\
  -Wl,--gc-sections

OBJS=$(SRC:.c=.o) $(ASRC:.S=.o)
DEPS=$(SRC:.c=.d)

all: $(TARGET)

-include $(DEPS)

%.o: %.c
	@echo " CC $<"
	@$(CC) -MMD $(CFLAGS) -o $@ -c $<

%.o: %.S
	@echo " AS $<"
	@$(AS) $(CFLAGS) -o $@ -c $<

.PHONY: clean all debug app

$(TARGET): $(OBJS)
	@echo " LINK $@"
	@$(LD) -o $@ $(LDFLAGS) $^

app:
	@$(MAKE) -C app clean all list

clean:
	@echo " CLEAN"
	@rm -fR $(OBJS) $(DEPS) $(TARGET)
