#include <stdio.h>
#include <assert.h>
#include "config.h"

int getline(char **lineptr, int *n, FILE *stream);
int atoi(const char *str);

int GetConfigInfo(char* configLocation, char processNames[128][256], int* numProcesses, int* killTime){
	FILE* configFile = fopen(configLocation, "r");
	if (configFile == NULL) assert(0);
	int read;
	int len = 127;
	*numProcesses = 0;
	char killTimeStr[128] = { 0 };
	char* pString = killTimeStr;
	read = getline(&pString, &len, configFile);
	for (int i = 0; i < 128; i++){
		pString = processNames[i];
		read = getline(&pString, &len, configFile);
		if (read == -1) break;
		(*numProcesses)++;
	}

	*killTime = atoi(killTimeStr);
	fclose(configFile);

	return 0;
}