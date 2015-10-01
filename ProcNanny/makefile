all: procnanny

processfinder.o: processfinder.c processfinder.h
	gcc -Wall -g -c -std=c99 processfinder.c

killer.o: killer.c killer.h
	gcc -Wall -g -c -std=c99 killer.c

config.o: config.c config.h
	gcc -Wall -g -c -std=c99 config.c

procnanny.o: procnanny.c
	gcc -Wall -g -c -std=c99 procnanny.c

procnanny: procnanny.o config.o killer.o processfinder.o
	gcc -Wall -g -std=c99 procnanny.o config.o killer.o processfinder.o -o procnanny

clean:
	rm *o procnanny

