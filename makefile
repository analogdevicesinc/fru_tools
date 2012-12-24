VERSION = 0
PATCHLEVEL = 8
SUBLEVEL = 0

#CFLAGS+= -g -O0 -Wall -ansi -pedantic
CFLAGS+= -O2 -Wall
#CC=i586-mingw32msvc-gcc
#EXE=.exe

MAIN_VERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))
ALL:
	$(CC) $(CFLAGS) -DVERSION=\"$(MAIN_VERSION)\" fru.c operating-system.c -o fru-dump$(EXE)

clean:
	rm -f fru-dump fru-dump.exe *.o
	rm -f *~
