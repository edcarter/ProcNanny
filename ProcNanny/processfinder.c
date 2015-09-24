#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct ProcessData {
	char[32] PID;
	char[32] CMD;
} ProcessData;

const size_t MaxNumberOfProcesses = 256;
const size_t MaxLineSize = 128;

int GetRunningProcesses(ProcessData* processes, int* numProcesses){

	char[MaxNumberOfProcesses][maxLineSize] psOutput = { 0 };
	GetPSOutput(psOutput, MaxNumberOfProcesses, MaxLineSize)
	psOutput++ //First line of output is headers, skip over.
	for (int i = 0; i < MaxNumberOfProcesses; i++){
		if (psOutput[i] == 0) break;
		ConstructProcess(&processes[i], psOutput[i]);
		*numProcesses++;
	}

	return 0;
}

int GetPSOutput(char** output, size_t maxProcess, size_t maxLineSize){
	FILE* output;
	int lineNumber = 0;

	output = popen("ps", "r");
	if (output == NULL){
		assert("Could Not Get PS Output");
		return -1;
	}
	while (fgets(output[lineNumber], maxLineSize-1, fp) != NULL){
		lineNumber++;
	}
	pclose(output);
	return 0;
}

int ConstructProcess(ProcessData* process, Char* psOutput){
	char[4][32] fields = { 0 };
	int fieldIndex = 0;
	int fieldCharacterIndex = 0;

	while(*psOutput != 0){
		while(*psOutput != 32){
			fields[fieldIndex][fieldCharacterIndex++] = *psOutput;
		}

		while(*psOutput++ == 32){} //eat all spaces
		fieldIndex++;
		fieldCharacterIndex = 0;
	}

	*process.PID = fields[0];
	*process.CMD = fields[3];

	return 0;

}
