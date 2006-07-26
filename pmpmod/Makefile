include ../config.mak
PSPSDK=$(shell psp-config --pspsdk-path)
PSPLIBSDIR=$(PSPSDK)/..

TARGET = PMPMOD
OBJS = pmpmod.o mod/csc.o mod/mem64.o mod/mestub.o mod/me.o mod/me_csc.o mod/buf_read.o mod/pmp_lib.o mod/pmp_read.o mod/time_math.o mod/pmp_decode.o mod/pmp_play.o mod/pmp.o mod/opendir.o mod/gu_funcs.o

INCDIR = .. ../libavformat ../libavcodec ../libavutil ./mod $(PSPLIBSDIR)/include ../libavcodec/mod
CFLAGS = -O3 -G0 -g -Wall -DHAVE_AV_CONFIG_H
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

USE_PSPSDK_LIBC =
LIBDIR = ..	$(PSPLIBSDIR)/lib
LDFLAGS =
LIBS = -lavformat -lavutil -lavcodec -lm -lpsppower -lpspaudio -lpspgu -lpsprtc -lpspusb -lpspusbstor -lpspusb_driver

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PMP Mod v1.02 M4g

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
