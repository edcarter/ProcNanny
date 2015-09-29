#include "processfinder.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

int strcmp(const char *str1, const char *str2);
int getpid(void);
int Kill(ProcessData process);
int IsOtherProcnanny(ProcessData process);

int main(){
	int maxNumberOfProcesses = GetMaxNumberOfProcesses();
	ProcessData* processes = (ProcessData*)malloc(maxNumberOfProcesses * sizeof(ProcessData));
	int processesRunning;
	if (GetRunningProcesses(processes, &processesRunning)){
		assert(0);
	}

	for (int i = 0; i < processesRunning; i++){
		if (IsOtherProcnanny(processes[i])){
			Kill(processes[i]);
		}
	}
	//Kill Other ProcNanny Processes

	//Read In Config File

	//Set Up Log File

	//Check What Processes out of config are actually running

	//Fork Child Processes for each monitor

	//Wait for mesages from child processes

	//Clean up Children

	//Log metadata and exit

}

int IsOtherProcnanny(ProcessData process){
	int thisPid = getpid();
	char thisPidString[32];
	if (!strcmp(process.CMD, "procnanny")){
		if (strcmp(thisPidString, process.PID)){ //If PIDs are different
			return 1;
		}
	}
	return 0;
}