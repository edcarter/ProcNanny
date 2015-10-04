/*
Copyright 2015 Elias Carter

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */


#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "memwatch.h"

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

ProcessData* GetProcessesToTrack(ProcessData* processesRunning, char processesInConfig[128][256], int inputCount, int* outputCount){
	*outputCount = 0;

	for (int i = 0; i < 128; i++){
		for (int j = 0; j < inputCount; j++){
			if (!strcmp(processesInConfig[i], processesRunning[j].CMD)){
				(*outputCount)++;
			}
		}
	}

	ProcessData* processesToTrack = (ProcessData*) calloc(*outputCount, sizeof(ProcessData));
	ProcessData* walkingProcessesToTrack = processesToTrack;

	for (int i = 0; i < 128; i++){
		for (int j = 0; j < inputCount; j++){
			if (!strcmp(processesInConfig[i], processesRunning[j].CMD)){
				*walkingProcessesToTrack++ = processesRunning[j];
			}
		}
	}

	return processesToTrack; //make sure to free this
}