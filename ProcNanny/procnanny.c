#include "processfinder.h"
#include "config.h"
#include "logger.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h> //might not need this

int strcmp(const char *str1, const char *str2);
int getpid(void);
int Kill(ProcessData process);
int IsOtherProcnanny(ProcessData process);
int sprintf(char *str, const char *format, ...);
void MonitorProcess(ProcessData process, int waitTime);

typedef struct MonitorData {
	int monitorPID;
	ProcessData* monitoredProcessData;
} MonitorData;

int main(int argc, char *argv[]){

	assert(argc >= 2);

	char* configLocation = argv[1];
	char* logPath = getenv("PROCNANNYLOGS");

	ProcessData data = {{0}};
	strcpy(data.CMD, "TESTCOMMAND\n");
	ReportProcessNotRunning(logPath, &data); //DELETE ME!

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

	MonitorData* monitors = (MonitorData*) calloc(numProcessesToMonitor, sizeof(MonitorData));


	//Fork Child Processes for each monitor
	int childPID = 1;
	ProcessData processToMonitor;
	for (int i = 0; i < numProcessesToMonitor; i++){
		monitors[i].monitoredProcessData = &processesToMonitor[i];
		processToMonitor = processesToMonitor[i];
		childPID = fork();
		if (childPID >= 0){
			if (childPID ==0){ // child process
				MonitorProcess(processToMonitor, killTime);
			} else { //parent process
				monitors[i].monitorPID = childPID;
			}
		} else {
			assert(0); //unable to fork
		}
	}

	//child should never get to this point
	assert(childPID);

	//Wait for mesages from child processes

	int exitStatus = 0;
	int exitedPID = 0;
	for (int i = 0; i < numProcessesToMonitor; i++){
		exitedPID = wait(&exitStatus);
	}

	//write to log that child has exited


	//Log metadata and exit
	free(monitors);
	free(processesToMonitor);
	free(processes);
}

void MonitorProcess(ProcessData process, int waitTime){
	sleep(waitTime);
	int killSuccess = Kill(process);
	if (killSuccess == -1){
		exit(EXIT_FAILURE);
	} else {
		exit(EXIT_SUCCESS);
	}
}


int IsOtherProcnanny(ProcessData process){
	int thisPid = getpid();
	char thisPidString[32] = {0};
	sprintf(thisPidString, "%d", thisPid);
	if (!strcmp(process.CMD, "procnanny\n")){
		if (strcmp(thisPidString, process.PID)){ //If PIDs are different
			return 1;
		}
	}
	return 0;
}