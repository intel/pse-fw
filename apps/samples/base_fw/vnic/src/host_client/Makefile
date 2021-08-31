# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DDEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

ccflags-y += $(DEBFLAGS)

PWD       := $(shell pwd)

all:
	$(CC) -o udp_service udp_service.c

clean:
	rm -rf *.o

