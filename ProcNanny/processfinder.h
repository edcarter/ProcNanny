struct ProcessData {
	char[32] PID;
	char[32] CMD;
} ProcessData;

int GetRunningProcesses(ProcessData* processes, int* numProcesses);