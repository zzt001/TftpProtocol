CC=gcc
CFLAGS=-I.
DEPS = tftp.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tftpclient: tftpclient.o tftp.o 
	gcc -o tftpclient tftpclient.o tftp.o -I.