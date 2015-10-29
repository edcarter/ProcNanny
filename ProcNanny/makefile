all: procnanny

memwatch.o: memwatch.c memwatch.h
	gcc -DMEMWATCH -DMW_STDIO -c -g memwatch.c

processfinder.o: processfinder.c processfinder.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g -std=c99 processfinder.c

killer.o: killer.c killer.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g -std=c99 killer.c

config.o: config.c config.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g -std=c99 config.c

logger.o: logger.c logger.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g -std=c99 logger.c

procnanny.o: procnanny.c
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g -std=c99 procnanny.c


procnanny: procnanny.o config.o killer.o processfinder.o logger.o memwatch.o
	gcc -Wall -DMEMWATCH -DMW_STDIO -std=c99 -g procnanny.o config.o killer.o processfinder.o logger.o memwatch.o -o procnanny

clean:
	rm *o procnanny memwatch.log

