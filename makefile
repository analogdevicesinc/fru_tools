

#CFLAGS+= -g -O0 -Wall -ansi -pedantic
CFLAGS+= -O2 -Wall
#CC=i586-mingw32msvc-gcc
#EXE=.exe
ALL:
	$(CC) $(CFLAGS) fru.c operating-system.c -o fru-dump$(EXE)

clean:
	rm -f fru-dump fru-dump.exe *.o
	rm -f *~
