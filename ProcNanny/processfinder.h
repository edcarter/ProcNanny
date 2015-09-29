typedef struct ProcessData {
	char PID[32];
	char CMD[32];
} ProcessData;

int GetRunningProcesses(ProcessData* processes, int* numProcesses);
int GetMaxNumberOfProcesses();