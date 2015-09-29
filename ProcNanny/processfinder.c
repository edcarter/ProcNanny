#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "processfinder.h"


const size_t MaxNumberOfProcesses = 256;
const size_t MaxLineSize = 128;

FILE *popen( const char *command, const char *modes);
void *memcpy(void *str1, const void *str2, size_t n);
int pclose(FILE *stream);
int GetPSOutput(char output[256][128], size_t maxProcess, size_t maxLineSize);
int ConstructProcess(ProcessData* process, char* psOutput);

int GetRunningProcesses(ProcessData* processes, int* numProcesses){

	char psOutput[256][128];
	*numProcesses = 0;
	GetPSOutput(psOutput, MaxNumberOfProcesses, MaxLineSize);
	for (int i = 0; i < MaxNumberOfProcesses; i++){
		if (psOutput[i+1][0] == 0) break;
		ConstructProcess(&processes[i], psOutput[i+1]);
		(*numProcesses)++;
	}

	return 0;
}

int GetMaxNumberOfProcesses(){
	return MaxNumberOfProcesses;
}

int GetPSOutput(char output[256][128], size_t maxProcess, size_t maxLineSize){
	FILE* foutput;
	int lineNumber = 0;

	foutput = popen("ps", "r");
	if (foutput == NULL){
		assert(0);
		return -1;
	}
	while (fgets(output[lineNumber], maxLineSize-1, foutput) != NULL){
		lineNumber++;
	}
	pclose(foutput);
	return 0;
}

int ConstructProcess(ProcessData* process, char* psOutput){
	char fields[4][32] = {{0}};
	int fieldIndex = 0;
	int fieldCharacterIndex = 0;

	while(*psOutput != 0){

		while(*psOutput == 32 && *psOutput != 0){psOutput++;} //eat all spaces
		while(*psOutput != 32 && *psOutput != 0){ //copy in field
			fields[fieldIndex][fieldCharacterIndex++] = *psOutput;
			psOutput++;
		}

		fieldIndex++;
		fieldCharacterIndex = 0;
	}

	memcpy(process->PID, fields[0], 32);
	memcpy(process->CMD, fields[3], 32);

	return 0;

}
