CC=gcc
CFLAGS=-lusb -lcrypto

.SILENT: iran clean
	
iran: main.o
	$(CC) *.o $(CFLAGS) -o iran

clean:
	rm -f *.o iran

