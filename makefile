

#CFLAGS+= -g -O0 -Wall -ansi
CFLAGS+= -O2 -Wall
#CC=gcc
ALL:
	$(CC) $(CFLAGS) fru.c operating-system.c -o fru-dump

clean:
	rm -f fru-dump *.o
	rm -f *~
