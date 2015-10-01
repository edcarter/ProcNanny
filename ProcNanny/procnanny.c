#include "processfinder.h"
#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

int strcmp(const char *str1, const char *str2);
int getpid(void);
int Kill(ProcessData process);
int IsOtherProcnanny(ProcessData process);
int sprintf(char *str, const char *format, ...);

int main(int argc, char *argv[]){

	assert(argc >= 2);

	char* configLocation = argv[1];

	//Get Running Processes
	int maxNumberOfProcesses = GetMaxNumberOfProcesses();
	ProcessData* processes = (ProcessData*)malloc(maxNumberOfProcesses * sizeof(ProcessData));
	int processesRunning;
	if (GetRunningProcesses(processes, &processesRunning)){
		assert(0);
	}

	//Kill Other ProcNanny Processes
	for (int i = 0; i < processesRunning; i++){
		if (IsOtherProcnanny(processes[i])){
			Kill(processes[i]);
		}
	}

	//Read In Config File
	int numProcessesInConfig;
	char configProcesses[128][256] = {{0}};
	int killTime;
	GetConfigInfo(configLocation, configProcesses, &numProcessesInConfig, &killTime);

	//Set Up Log File (TODO)

	//Check What Processes out of config are actually running
	int numProcessesToMonitor;
	ProcessData* processesToMonitor = GetProcessesToTrack(processes, configProcesses, processesRunning, &numProcessesToMonitor);

	//Fork Child Processes for each monitor

	//Wait for mesages from child processes

	//Clean up Children

	//Log metadata and exit
	free(processesToMonitor);
	free(processes);
}

int IsOtherProcnanny(ProcessData process){
	int thisPid = getpid();
	char thisPidString[32];
	sprintf(thisPidString, "%d", thisPid);
	if (!strcmp(process.CMD, "procnanny")){
		if (strcmp(thisPidString, process.PID)){ //If PIDs are different
			return 1;
		}
	}
	return 0;
}