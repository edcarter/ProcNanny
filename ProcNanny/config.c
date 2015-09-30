#include <stdio.h>
#include <assert.h>
#include "config.h"

int getline(char **lineptr, int *n, FILE *stream);
int atoi(const char *str);

int GetConfigInfo(char* configLocation, char processNames[128][256], int* killTime){
	FILE* configFile = fopen(configLocation, "r");
	if (configFile == NULL) assert(0);
	int read;
	int len = 127;
	char killTimeStr[128];
	read = getline(&killTimeStr, &len, configFile);
	for (int i = 0; i < 128; i++){
		read = getline(&processNames[i], &len, configFile);
		if (read == -1) break;
	}

	*killTime = atoi(killTimeStr);
	fclose(configFile);

	return 0;
}