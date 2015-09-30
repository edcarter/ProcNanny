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

	char configProcesses[128][256] = {0};
	int killTime;


	//Read In Config File
	GetConfigInfo(configLocation, configProcesses, &killTime);

	//Set Up Log File

	//Check What Processes out of config are actually running

	//Fork Child Processes for each monitor

	//Wait for mesages from child processes

	//Clean up Children

	//Log metadata and exit
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