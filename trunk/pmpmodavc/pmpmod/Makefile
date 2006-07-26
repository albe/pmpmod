include ../config.mak
PSPSDK=$(shell psp-config --pspsdk-path)
PSPLIBSDIR=$(PSPSDK)/..

TARGET = PMPMODAVC
OBJS = pmpmod.o mod/aspect_ratio.o mod/avc.o mod/cpu_clock.o mod/me.o mod/mem64.o mod/mestub.o mod/mpegbase.o mod/opendir.o mod/pmp.o mod/pmp_decode.o mod/pmp_file.o mod/pmp_gu.o mod/pmp_interface.o mod/pmp_play.o mod/pmp_read.o mod/texture_subdivision.o \
       mod/gu_font.o mod/pmp_parse.o mod/pmp_microdvd.o mod/pmp_subrip.o mod/valloc.o

INCDIR = .. ../libmpeg ../libavformat ../libavcodec ../libavutil ./mod $(PSPLIBSDIR)/include ../libavcodec/mod
CFLAGS = -O3 -G0 -g -Wall -DHAVE_AV_CONFIG_H
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

USE_PSPSDK_LIBC =
LIBDIR = ..	$(PSPLIBSDIR)/lib
LDFLAGS =
LIBS = -lavformat -lavutil -lavcodec -lm -lpsppower -lpspaudio -lpspgu -lpspmpeg

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PMP Mod AVC
PSP_EBOOT_ICON = pack-pbp/ICON0.PNG
PSP_EBOOT_PIC1 = pack-pbp/PIC1.PNG

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak