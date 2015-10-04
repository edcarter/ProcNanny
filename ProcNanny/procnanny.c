#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h> 
#include "processfinder.h"
#include "config.h"
#include "logger.h"
#include "memwatch.h"

typedef struct MonitorData {
	int monitorPID;
	ProcessData* monitoredProcessData;
} MonitorData;

int strcmp(const char *str1, const char *str2);
int getpid(void);
int Kill(ProcessData process);
int IsOtherProcnanny(ProcessData process);
int sprintf(char *str, const char *format, ...);
void MonitorProcess(ProcessData process, int waitTime);
ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors);
int LogIfProcessNotRunning(ProcessData* runningProcesses, char configProcesses[128][256], int numConfigProcesses, int numRunningProcesses, char* logLocation);


int main(int argc, char *argv[]){

	assert(argc >= 2);

	char* configLocation = argv[1];
	char* logPath = getenv("PROCNANNYLOGS");

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


	//Check What Processes out of config are actually running
	int numProcessesToMonitor;
	ProcessData* processesToMonitor = GetProcessesToTrack(processes, configProcesses, processesRunning, &numProcessesToMonitor);
	LogIfProcessNotRunning(processesToMonitor, configProcesses, numProcessesInConfig, numProcessesToMonitor, logPath);

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
				free(monitors);
				free(processesToMonitor);
				free(processes);
				MonitorProcess(processToMonitor, killTime); //this does not return
			} else { //parent process
				monitors[i].monitorPID = childPID;
				ReportMonitoringProcess(logPath, &processToMonitor);
			}
		} else {
			assert(0); //unable to fork
		}
	}

	//child should never get to this point
	assert(childPID); //childPID == 0 if the child is running this code

	//Wait for mesages from child processes
	int totalKilled = 0;
	int exitStatus = 0;
	int exitedPID = 0;
	for (int i = 0; i < numProcessesToMonitor; i++){
		exitedPID = wait(&exitStatus);
		if (exitStatus == EXIT_SUCCESS){
			totalKilled++;
			ProcessData* monitoredProcess = GetMonitoredProcess(exitedPID, monitors, numProcessesToMonitor);
			ReportProcessKilled(logPath, monitoredProcess, killTime);
		} else {
			assert(0); //child did not exit successfully
		}
	}

	//Log metadata and exit
	ReportTotalProcessesKilled(logPath, totalKilled);

	FlushLogger(logPath);
	free(monitors);
	free(processesToMonitor);
	free(processes);
}

ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors){
	for (int i = 0; i < numMonitors; i++){
		if (monitors[i].monitorPID == monitorPID) {
			return monitors[i].monitoredProcessData;
		}
	}
	return NULL;
}

int LogIfProcessNotRunning(ProcessData* runningProcesses, char configProcesses[128][256], int numConfigProcesses, int numRunningProcesses, char* logLocation){
	int found = 0;
	for (int i = 0; i < numConfigProcesses; i++){
		char* cmd = configProcesses[i];
		for (int j = 0; j < numRunningProcesses; j++){
			if (!strcmp(cmd, runningProcesses[i].CMD)){
				found = 1;
				break;
			}
		}
		if (!found){
			ProcessData notRunningProcessData = {{0}};
			strcpy(notRunningProcessData.CMD, cmd);
			ReportProcessNotRunning(logLocation, &notRunningProcessData);
		}
		found = 0;
	}
	return 0;
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