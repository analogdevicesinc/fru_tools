VERSION = 0
PATCHLEVEL = 8
SUBLEVEL = 0

#CFLAGS+= -g -O0 -Wall -ansi -pedantic
CFLAGS+= -O2 -Wall
MING_CC=i586-mingw32msvc-gcc
MING_INSTALL = $(shell expr `which $(MING_CC) | wc -l` \>= 1)

MAIN_VERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))

ALL:
	$(CC) $(CFLAGS) -DVERSION=\"$(MAIN_VERSION)\" fru.c operating-system.c -o fru-dump$(EXE)

Win:
ifeq "$(MING_INSTALL)" "1"
	$(MING_CC) $(CFLAGS) -DVERSION=\"$(MAIN_VERSION)\" fru.c operating-system.c -o fru-dump.exe
else
	echo "Couldn't find mingw32 toolchain"
endif

clean:
	rm -f fru-dump fru-dump.exe *.o
	rm -f *~
