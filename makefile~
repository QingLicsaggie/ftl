CC = gcc

all: ftlsim

ftlsim: getaddr.o ftlsim.o  main.o
	$(CC) -o ftlsim getaddr.o ftlsim.o main.o

getaddr.o: getaddr.c getaddr.h 
	$(CC) -c getaddr.c

ftlsim.o: ftlsim.c  ftlsim.h getaddr.h
	$(CC) -c ftlsim.c

main.o: main.c ftlsim.h getaddr.h
	$(CC) -c main.c

clean:
	/bin/rm -f *.o ftlsim
