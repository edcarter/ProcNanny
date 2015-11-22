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

//int getline(char **lineptr, int *n, FILE *stream);
void ConstructConfigData(char line[256], ConfigData* configData);
int atoi(const char *str);

int GetConfigInfo(char* configLocation, ConfigData* configs, int* numProcesses){
	FILE* configFile = fopen(configLocation, "r");
	if (configFile == NULL) assert(0);
	int read, i;
	int len = 256;
	*numProcesses = 0;
	char String[256] = {0};
	char* pString = String;

	for (i = 0; i < 128; i++){
		read = getline(&pString, (size_t *) &len, configFile);
		if (read == -1) break;
		ConstructConfigData(pString, configs + i);
		(*numProcesses)++;
	}
	fclose(configFile);

	return 0;
}

void ConstructConfigData(char line[256], ConfigData* configData){
	int space_index = 0;
	for( ; space_index < 256; space_index++){
		if (line[space_index] == ' ') break;
	}
	assert(space_index < 256);
	memset(configData->CMD, 0, 256);
	memcpy(configData->CMD, line, space_index);
	configData->CMD[space_index] = '\n';
	int newline_index = space_index;

	for ( ; newline_index < 256; ){
		newline_index++;
		if (line[newline_index] == '\n') break;
	}
	assert(space_index < 256);
	char timeString[256] = {0};
	memcpy(timeString, line + space_index + 1, newline_index - space_index); //sub by 1
	configData->killTime = atoi(timeString);
}

ProcessData* GetProcessesToTrack(ProcessData* processesRunning, ConfigData* configs, int inputCount, int* outputCount){
	*outputCount = 0;
	int i, j;

	for (i = 0; i < 128; i++){
		for (j = 0; j < inputCount; j++){
			if (!strcmp(configs[i].CMD, processesRunning[j].CMD)){
				(*outputCount)++;
			}
		}
	}

	ProcessData* processesToTrack = (ProcessData*) calloc(*outputCount, sizeof(ProcessData));
	ProcessData* walkingProcessesToTrack = processesToTrack;

	for (i = 0; i < 128; i++){
		for (j = 0; j < inputCount; j++){
			if (!strcmp(configs[i].CMD, processesRunning[j].CMD)){
				*walkingProcessesToTrack = processesRunning[j];
				walkingProcessesToTrack->killTime = configs[i].killTime;
				walkingProcessesToTrack++;
			}
		}
	}

	return processesToTrack; //make sure to free this
}