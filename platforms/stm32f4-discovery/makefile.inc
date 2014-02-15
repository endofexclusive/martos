# Copyright (c) 2014, Martin Åberg All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The names of the copyright holder(s) may not be used to endorse or
#    promote products derived from this software without specific prior
#    written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AS=arm-none-eabi-gcc
OBJDUMP=arm-none-eabi-objdump

LDSCRIPT = $(PLATFORM_ROOT)/stm32f4.ld
LDFLAGS+= -nostartfiles -T$(LDSCRIPT) -mcpu=cortex-m4 -mthumb -Wl,-Map=linker.map -Wl,--cref

CFLAGS+= -std=c99
CFLAGS+= -mcpu=cortex-m4 -mthumb
CFLAGS+= -DHSE_VALUE=8000000
CFLAGS+= -I$(PLATFORM_ROOT)
CFLAGS+= -I$(MARTOS_ROOT)/src
CFLAGS+= -I$(MARTOS_ROOT)/include
CFLAGS+= -I$(PLATFORM_ROOT)/cmsis

ASFLAGS+=$(CFLAGS)
ASFLAGS+=-c

# Define ONE_NAMESPACE to compile most of the kernel in one
# compilation unit.
#ONE_NAMESPACE=1

ifdef ONE_NAMESPACE
    CFLAGS+=-DMARTOS_NAMESPACE
    OBJS+=martos.o
else
    OBJS+=data.o
    OBJS+=platform.o
    OBJS+=list.o
    OBJS+=task.o
    OBJS+=msgport.o
endif

OBJS+=system_stm32f4xx.o
OBJS+=reset_handler.o
OBJS+=pendsv_handler.o

# Search path for MARTOS
vpath %.c $(MARTOS_ROOT)/src

# Search path for PLATFORM
vpath %.c $(PLATFORM_ROOT)
vpath %.c $(PLATFORM_ROOT)/cmsis
vpath %.s $(PLATFORM_ROOT)


$(NAME).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) 
	arm-none-eabi-size -d $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(OBJS:.o=.d) *.elf linker.map

sources = $(OBJS:.o=.c)
-include $(sources:.c=.d)

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

