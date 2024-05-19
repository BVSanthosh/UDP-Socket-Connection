CC=gcc
LDFLAGS=-lm

packets: slurpe-probe.o UdpSocket.o
	$(CC) -o packets slurpe-probe.o UdpSocket.o

slurpe-probe.o: slurpe-probe.c slurpe-probe.h
	$(CC) -c slurpe-probe.c

UdpSocket.o: UdpSocket.c UdpSocket.h
	$(CC) -c UdpSocket.c

clean:
	rm -f myprogram slurpe-probe.o UdpSocket.o