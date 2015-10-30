/*
Copyright 2015 Elias Carter

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h> 
#include "processfinder.h"
#include "config.h"
#include "logger.h"
#include "memwatch.h"

typedef struct MonitorData {
	int monitorPID;
	int pipe[2];
	ProcessData* monitoredProcessData;
} MonitorData;

typedef struct PipeData {
	char type[4]; //NEW or EXT
	ProcessData monitoredProcess;
} PipeData;

int strcmp(const char *str1, const char *str2);
int getpid(void);
int Kill(ProcessData process);
int IsOtherProcnanny(ProcessData process);
int sprintf(char *str, const char *format, ...);
void MonitorProcess(ProcessData process);
ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors);
int LogIfProcessNotRunning(ProcessData* runningProcesses, ConfigData* configProcesses, int numConfigProcesses, int numRunningProcesses, char* logLocation);

int exiting = 0;
int reReadConfig = 0;

int main(int argc, char *argv[]){
	signal(SIGINT, HandleSigint);
	signal(SIGHUP, HandleSighup);

	assert(argc >= 2);

	char* configLocation = argv[1];
	char* logPath = getenv("PROCNANNYLOGS");

	//Get Running Processes
	int maxNumberOfProcesses = GetMaxNumberOfProcesses();
	ProcessData* processes = (ProcessData*)calloc(maxNumberOfProcesses, sizeof(ProcessData));
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
	ConfigData*  configProcesses = (ConfigData*)malloc(maxNumberOfProcesses * sizeof(ConfigData));
	GetConfigInfo(configLocation, configProcesses, &numProcessesInConfig);


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
				free(configProcesses);
				RunChild();
				MonitorProcess(processToMonitor); //this does not return
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
			ReportProcessKilled(logPath, monitoredProcess);
		} else {
			//do nothing, child did not sucessfully kill process
		}
	}

	while(!exiting){

	}

	//Log metadata and exit
	ReportTotalProcessesKilled(logPath, totalKilled);

	FlushLogger(logPath);
	free(monitors);
	free(processesToMonitor);
	free(processes);
	free(configProcesses);
}

void HandleSigint(int signal){
	exiting = 1;
}

void HandleSigHup(int signal){
	reReadConfig = 1;
}

ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors){
	for (int i = 0; i < numMonitors; i++){
		if (monitors[i].monitorPID == monitorPID) {
			return monitors[i].monitoredProcessData;
		}
	}
	return NULL;
}

int LogIfProcessNotRunning(ProcessData* runningProcesses, ConfigData* configProcesses, int numConfigProcesses, int numRunningProcesses, char* logLocation){
	int found = 0;
	for (int i = 0; i < numConfigProcesses; i++){
		char* cmd = configProcesses[i].CMD;
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

int MonitorProcess(ProcessData process){
	sleep(process.killTime);
	int killSuccess = Kill(process);
}

void RunChild(ProcessData process, int pipe[2]){
	PipeData data = {0};
	while (true){	
		int killed = MonitorProcess(process);
		if (killed){
		//need to notify parent when process is killed?
		}
		read(pipe[0], data, sizof(PipeData)); //this should block if the pipe was created as blocking
		process = data.monitoredProcess;
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