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

NAME = netproj
MARTOS_ROOT = ..
PLATFORM_ROOT = ../platforms/stm32f4-discovery
LWIP_ARCH = ../lwip_martos
LWIP_SRC = ../lwip-1.4.1/src
LWIP_LIB = $(PLATFORM_ROOT)/lwip/lwip.a
OBJS+= n.o
OBJS+= udpecho.o

CFLAGS = -O0
#CFLAGS+= -DNDEBUG
CFLAGS+= -g
CFLAGS+= -Wall #-Wextra -Wpedantic

LDLIBS+= $(LWIP_LIB)

#CFLAGS+= -I$(PLATFORM_ROOT)
#CFLAGS+= -I$(MARTOS_ROOT)/src
#CFLAGS+= -I$(MARTOS_ROOT)/include
CFLAGS+= -I$(LWIP_SRC)/include -I$(LWIP_SRC)/include/ipv4 -I$(LWIP_ARCH)/include
CFLAGS+=-DLWIP_DEBUG

all: $(NAME).elf
include $(PLATFORM_ROOT)/makefile.inc

.PHONY: test
test: $(NAME).elf
	st-util > /dev/null &
	arm-none-eabi-gdb -x=gdbcommands
	killall st-util
