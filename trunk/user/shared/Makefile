ifndef ROOTDIR
ROOTDIR=../..
endif

include boards.mk
include cflags.mk

CFLAGS += -fPIC
CFLAGS += -Wall -I. -I./include -I$(ROOTDIR)/$(LINUXDIR)/include/nvram

LDFLAGS += -L.

OBJS := shutils.o netutils.o rtutils.o defaults.o nvram_linux.o nvram_batch.o nvram_cache.o cmd_exec.o cmd_batch.o logging.o service_table.o notify_rc.o bin_sem_asus.o flash_mtd.o pids.o config_validate.o

all: libshared.so

libshared.so: $(OBJS)
	$(LD) -shared -o $@ $^

romfs:
	$(ROMFSINST) -S /lib/libshared.so

clean:
	rm -f *.o libshared.so
