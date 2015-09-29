#include "processfinder.h"
#include <stdlib.h>
#include <assert.h>

int main(){
	int maxNumberOfProcesses = GetMaxNumberOfProcesses();
	ProcessData* processes = (ProcessData*)malloc(maxNumberOfProcesses * sizeof(ProcessData));
	int processesRunning;
	if (GetRunningProcesses(processes, &processesRunning)){
		assert(0);
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