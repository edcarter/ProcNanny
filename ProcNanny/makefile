all: procnanny

processfinder.o: processfinder.c processfinder.h
	gcc -Wall -g -c -std=c99 processfinder.c

killer.o: killer.c killer.h
	gcc -Wall -g -c -std=c99 killer.c

config.o: config.c config.h
	gcc -Wall -g -c -std=c99 config.c

logger.o: logger.c logger.h
	gcc -Wall -g -c -std=c99 logger.c

procnanny.o: procnanny.c
	gcc -Wall -g -c -std=c99 procnanny.c


procnanny: procnanny.o config.o killer.o processfinder.o logger.o
	gcc -Wall -g -std=c99 procnanny.o config.o killer.o processfinder.o logger.o -o procnanny

clean:
	rm *o procnanny

