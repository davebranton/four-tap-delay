# put your *.o targets here, make should handle the rest!

SRCS = main.cpp system_stm32f4xx.c

# all the files will be generated with this name (main.elf, main.bin, main.hex, etc)

PROJ_NAME=main

# that's it, no need to change anything below this line!

###################################################

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS  = -O3 -Wall -Tstm32_flash.ld  -Isrc
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=c++14 -DHSE_VALUE=8000000


CMD_CC=g++

###################################################

vpath %.cpp src
vpath %.c src
vpath %.a lib

ROOT=$(shell pwd)

CFLAGS += -Iinc -Ilib -Ilib/inc -specs=nosys.specs  -g -Istatic_math
CFLAGS += -Ilib/inc/core -Ilib/inc/peripherals  -Wno-misleading-indentation -O3  -DHSE_VALUE=8000000

SRCS += lib/startup_stm32f4xx.s # add startup file to build

OBJS = $(SRCS:.cpp=.o)

###################################################

.PHONY: lib  proj

all: lib  proj

lib:
	$(MAKE) -C lib

proj: 	$(PROJ_NAME).elf

$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ -Llib  -lstm32f4
	$(OBJCOPY) -O ihex $(PROJ_NAME).elf $(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin


$(PROJ_NAME).s: main.cpp $(HDRS)
	$(CC) $(CFLAGS) $^ -fverbose-asm  -S -o $@  -Llib  -lstm32f4

clean:
	$(MAKE) -C lib clean
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).hex
	rm -f $(PROJ_NAME).bin

debug: main.elf
	arm-none-eabi-gdb -ex "tar ext :4242" -ex "load main.elf" -ex "file main.elf" 

run: main.elf
	arm-none-eabi-gdb -batch -x gdb_cmnds.txt