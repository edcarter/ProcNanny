#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct Process {
	long int PID;
	char* CMD;
} Process;

const int MaxNumberOfProcesses = 256;
const size_t MaxLineSize = 128;

int GetRunningProcesses(Process* processes, int* numProcesses){

	char[MaxNumberOfProcesses][maxLineSize] psOutput = { 0 };
	GetPSOutput(psOutput, MaxNumberOfProcesses, maxLineSize)
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