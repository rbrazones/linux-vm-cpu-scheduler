# Ryan Brazones - rbrazones@gmail.com 
#
# File: Makefile
# Description: Run "make" to build the vcpu scheduler project

CC = gcc
CFLAGS := -Wall -Werror --std=gnu99 -g3

ARCH := $(shell uname)
ifneq ($(ARCH),Darwin)
	LDFLAGS += -lvirt -lm
endif

all: vcpu_scheduler

vcpu_scheduler: vcpu_scheduler.c math_func.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS) 

.PHONY: clean

clean: 
	rm -rf *.o vcpu_scheduler
