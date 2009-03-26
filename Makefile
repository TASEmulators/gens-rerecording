OSTARSCREAM=\
common/starscream/main68k.o\
common/starscream/sub68k.o

OCOMMONFILES=\
common/core/cpu/68k/cpu_68k.o\
common/core/cpu/sh2/cpu_sh2.o\
common/core/cpu/sh2/sh2a.o\
common/core/cpu/sh2/sh2.o\
common/core/cpu/z80/cpu_z80.o\
common/core/cpu/z80/z80.o\
common/core/gfx/blit.o\
common/core/gfx/gfx_cd.o\
common/core/io/io.o\
common/core/mem/mem_m68k.o\
common/core/mem/mem_s68k.o\
common/core/mem/mem_sh2.o\
common/core/mem/mem_z80.o\
common/core/misc/misc.o\
common/core/sound/pcm.o\
common/core/sound/psg.o\
common/core/sound/pwm.o\
common/core/sound/ym2612.o\
common/core/vdp/vdp_32x.o\
common/core/vdp/vdp_io.o\
common/core/vdp/vdp_rend.o\
common/debug/debug.o\
common/debug/z80dis.o\
common/debug/m68kd.o\
common/debug/sh2d.o\
common/mp3_dec/dct64_i386.o\
common/mp3_dec/layer3.o\
common/mp3_dec/common.o\
common/mp3_dec/decode_i386.o\
common/mp3_dec/interface.o\
common/mp3_dec/tabinit.o\
common/segacd/cd_file.o\
common/segacd/cd_sys.o\
common/segacd/cdda_mp3.o\
common/segacd/cd_aspi.o\
common/segacd/lc89510.o\
common/unzip.o\
common/wave.o\
common/ggenie.o\
common/rom.o\
common/save.o\
common/scrshot.o

OFILES=\
linux/port.o\
linux/timer.o\
linux/g_ddraw.o\
linux/g_dinput.o\
linux/g_dsound.o\
linux/g_main.o\
linux/gens.o\

OGTK=\
linux/glade/callbacks.o\
linux/glade/interface.o\
linux/glade/support.o

GTKCFLAGS=-DWITH_GTK `pkg-config gtk+-2.0 --cflags`

#uncomment this if you want to enable GTK support (default)
#CFLAGS=-Wall -O3 -D__PORT__  `sdl-config --cflags` -I. $(GTKCFLAGS)
#uncomment this if if you want to disable GTK support
CFLAGS=-Wall -g -D__PORT__  `sdl-config --cflags` -I.

CXXFLAGS=$(CFLAGS)

GTKLDFLAGS=`pkg-config gtk+-2.0 --libs`

#uncomment this if you want to enable GTK support (default)
#LDFLAGS=-lm `sdl-config --libs` -lz -lstdc++ -s $(GTKLDFLAGS)
#uncomment this if you want to disable GTK support
LDFLAGS=-lm `sdl-config --libs` -lz -lstdc++ -s

NASMFLAGS=-D__GCC2 -f elf -w-orphan-labels
CC=gcc
CXX=g++
TARGET=gens

#uncomment this if you want to enable GTK support (default)
#ALLFILES = $(OCOMMONFILES) $(OFILES) $(OSTARSCREAM) $(OGTK)
#uncomment this if you want to disable GTK support
ALLFILES = $(OCOMMONFILES) $(OFILES) $(OSTARSCREAM)

.SUFFIXES:
.SUFFIXES:.c .cpp .asm .o

#ALLFILES += xpatch-lib.a

all: $(ALLFILES)
	$(CC) $^ $(LDFLAGS) -o $(TARGET)

Starscream/Main68k/main68k.o: common/starscream/star_main68k
	common/starscream/star_main68k common/starscream/temp_main68k.asm -hog -name main68k_
	nasm -f elf -w-orphan-labels common/starscream/temp_main68k.asm -o$@
	rm -f common/starscream/temp_main68k.asm

Starscream/Sub68k/sub68k.o: common/starscream/star_sub68k
	common/starscream/star_sub68k common/starscream/temp_sub68k.asm -hog -name sub68k_
	nasm -f elf -w-orphan-labels common/starscream/temp_sub68k.asm -o$@
	rm -f common/starscream/temp_sub68k.asm

Starscream/Main68k/star: common/starscream/star_main68k.c
	gcc -o$@ $^
Starscream/Sub68k/star: common/starscream/star_sub68k.c
	gcc -o$@ $^
	
.c.o:
.cpp.o:
.asm.o:
	nasm $(NASMFLAGS) $^ -o$@

clean:
	rm -rf $(OFILES) $(OGTK) $(TARGET)

extra-clean: clean
	rm -f common/starscream/star_main68k common/starscream/star_sub68k $(OSTARSCREAM)
	
