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

# This file is hard coded to the STM32F4-DISCOVERY port of MARTOS.

MARTOS_ROOT=../../..
PLATFORM_ROOT = ..
LWIP_SRC=$(MARTOS_ROOT)/lwip-1.4.1/src
LWIP_ARCH=$(MARTOS_ROOT)/lwip_martos
 
CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AR=arm-none-eabi-ar
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size

CFLAGS = -O0
#CFLAGS+= -DNDEBUG
CFLAGS+= -g
CFLAGS+= -Wall -Wextra -Wpedantic
CFLAGS+= -std=c99
CFLAGS+= -mcpu=cortex-m4 -mthumb
CFLAGS+= -I$(PLATFORM_ROOT)
CFLAGS+= -I$(MARTOS_ROOT)/src
CFLAGS+= -I$(MARTOS_ROOT)/include
CFLAGS+= -I$(LWIP_SRC)/include -I$(LWIP_SRC)/include/ipv4 -I$(LWIP_ARCH)/include
CFLAGS+=-DLWIP_DEBUG


OBJS+=tcpip.o
OBJS+=timers.o
OBJS+=etharp.o
OBJS+=ip.o
OBJS+=memp.o
OBJS+=init.o
OBJS+=pbuf.o
OBJS+=mem.o
OBJS+=ip_frag.o
OBJS+=ip_addr.o
OBJS+=stats.o
OBJS+=def.o
OBJS+=netif.o
OBJS+=inet_chksum.o
OBJS+=raw.o
OBJS+=udp.o
OBJS+=tcp.o
OBJS+=tcp_in.o
OBJS+=tcp_out.o
OBJS+=icmp.o
OBJS+=sys_arch.o
OBJS+=api_lib.o
OBJS+=api_msg.o
OBJS+=netbuf.o

vpath %.c $(LWIP_SRC)/core
vpath %.c $(LWIP_SRC)/api
vpath %.c $(LWIP_SRC)/netif
vpath %.c $(LWIP_SRC)/core/ipv4
vpath %.c $(LWIP_ARCH)

lwip.a: $(OBJS)
	$(AR) -r $@ $^
	$(SIZE) -d -t $@
	touch .

lwip.list: lwip.a
	$(OBJDUMP) -d $< > $@

lwip.data: lwip.a
	$(OBJDUMP) -s -t -j.data -j.rodata $< > $@

.PHONY: clean
clean:
	rm -f lwip.a $(OBJS) $(OBJS:.o=.d)

sources = $(OBJS:.o=.c)
-include $(sources:.c=.d)

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

#lwip-1.4.1.zip:
#	wget http://download.savannah.gnu.org/releases/lwip/lwip-1.4.1.zip

#lwip-1.4.1: lwip-1.4.1.zip
#	unzip lwip-1.4.1.zip
#	touch lwip-1.4.1

