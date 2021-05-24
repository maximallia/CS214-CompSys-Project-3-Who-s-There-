all: KKJserver

KKJserver: Asst3.c
	gcc -Wall -o KKJserver Asst3.c

clean:
	rm -f KKJserver *.o
