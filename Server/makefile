all: procnanny.server procnanny.client

memwatch.o: memwatch.c memwatch.h
	gcc -DMEMWATCH -DMW_STDIO -c -g memwatch.c

config.o: config.c config.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g config.c

logger.o: logger.c logger.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g logger.c

killer.o: killer.c killer.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g killer.c

processfinder.o: processfinder.c processfinder.h
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g processfinder.c 

procnanny.server.o: procnanny.server.c
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g procnanny.server.c

procnanny.client.o: procnanny.client.c
	gcc -Wall -DMEMWATCH -DMW_STDIO -c -g procnanny.client.c

procnanny.server: procnanny.server.o config.o logger.o memwatch.o
	gcc -Wall -DMEMWATCH -DMW_STDIO -g procnanny.server.o config.o logger.o memwatch.o -o procnanny.server

procnanny.client: procnanny.client.o memwatch.o killer.o processfinder.o
	gcc -Wall -DMEMWATCH -DMW_STDIO -g procnanny.client.o memwatch.o killer.o processfinder.o -o procnanny.client

clean:
	rm *o procnanny.server procnanny.client

