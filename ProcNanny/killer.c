#include <stdlib.h>
#include <assert.h>
#include "killer.h"

int sprintf(char *str, const char *format, ...);

int Kill(ProcessData process){
	//format string to kill pid
	char killCommand[64] = { 0 };
	sprintf(killCommand, "kill %d", atoi(process.PID));

	int rv = system(killCommand);
	assert(rv != -1);
	return rv;
}