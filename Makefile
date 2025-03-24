CC=gcc
LDFLAGS=-lusb -lcrypto

.SILENT: iran clean
	
iran: main.o
	$(CC) *.o $(CFLAGS) $(LDFLAGS) -o iran

clean:
	rm -f *.o iran
