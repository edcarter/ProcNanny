all: procnanny.server procnanny.client

procnanny.server.o: procnanny.server.c
	gcc -Wall -c -g -std=c99 procnanny.server.c

procnanny.client.o: procnanny.client.c
	gcc -Wall -c -g -std=c9 procnanny.client.c

procnanny.server: procnanny.server.o
	gcc -Wall -std=c99 -g procnanny.server.o -o procnanny.server

procnanny.client: procnanny.client.o
	gcc -Wall -std=c99 -g procnanny.client.o -o procnanny.client

clean:
	rm *o procnanny.server procnanny.client

