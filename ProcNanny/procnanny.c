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
#define _GNU_SOURCE

#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h> 
#include "processfinder.h"
#include "config.h"
#include "logger.h"
#include "memwatch.h"

#define MAX_PROCESSES 256

typedef struct MonitorData {
	int monitorPID;
	int read_pipe[2]; //pipe for reading from child
	int write_pipe[2]; //pipe for writing to child
	//ProcessData* monitoredProcessData;
	ProcessData monitoredProcessData;
} MonitorData;

typedef struct PipeData {
	char type[4]; //NEW, EXT
	//NEW notify child new process to monitor
	//EXT notify child to exit
	//KIL notify parent that process was killed
	//TIM notify parent that process timed out early
	ProcessData monitoredProcess;
} PipeData;

int strcmp(const char *str1, const char *str2);
void KillOtherProcnanny();
int getpid(void);
int Kill(ProcessData process);
int IsOtherProcnanny(ProcessData process);
int sprintf(char *str, const char *format, ...);
int MonitorProcess(ProcessData process, int* exiting);
ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors);
int LogIfProcessNotRunning(ProcessData* runningProcesses, ConfigData* configProcesses, int numConfigProcesses, int numRunningProcesses, char* logLocation);
ProcessData* GetProcessesNotMonitored(ProcessData* runningProcesses, int numRunningProcesses, MonitorData* monitors, int numMonitors, int* numNotMonitored);
ProcessData* GetProcessesToMonitor(char* configLocation, int* numProcesses, MonitorData* monitors, int numMonitors, ConfigData* configProcesses, int numConfigProcesses);
int ReadThroughChildren(MonitorData* monitors, int numMonitors);
void DelegateMonitorProcess(ProcessData* processToMonitor, int numProcessesToMonitor, MonitorData* monitors, int* numMonitors);
int GetIndexOfReadyMonitor(MonitorData* monitors, int numMonitors);
ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors);
void RunChild(ProcessData process, int read_pipe[2], int write_pipe[2]);
void HandleSigint(int signal);
void HandleSigHup(int signal);
void HangleSigintChild(int signal);

int exiting = 0;
int reReadConfig = 1;
ProcessData* processesToMonitor = NULL;
MonitorData* monitors = NULL;
char* logPath;
char* configLocation;
int totalKilled = 0; 
ConfigData* configProcesses = NULL;
ProcessData* processesNotMonitored = NULL;

int main(int argc, char *argv[]){
	signal(SIGINT, HandleSigint);
	signal(SIGHUP, HandleSigHup);
	assert(argc >= 2);
	logPath = getenv("PROCNANNYLOGS");
	ReportParentPidFile(logPath, getpid());
	ReportParentPid(stdout, getpid());
	configLocation = argv[1];
	monitors = (MonitorData*) calloc(MAX_PROCESSES, sizeof(MonitorData));
	int numMonitors = 0;

	KillOtherProcnanny();
	configProcesses = (ConfigData*)calloc(MAX_PROCESSES, sizeof(ConfigData));
	int numProcessesInConfig;
	//main while loop
	while(!exiting){
		if (reReadConfig){
			GetConfigInfo(configLocation, configProcesses, &numProcessesInConfig);
			free (processesToMonitor);
			processesToMonitor = NULL;		
			reReadConfig--;
			ReportSighupCaughtFile(logPath, configLocation);
			ReportSighupCaught(stdout, configLocation);
		}
		int numProcessesToMonitor = 0;
		processesToMonitor = GetProcessesToMonitor(configLocation, &numProcessesToMonitor, monitors, numMonitors, configProcesses, numProcessesInConfig);
		DelegateMonitorProcess(processesToMonitor, numProcessesToMonitor, monitors, &numMonitors);
		int killed = ReadThroughChildren(monitors, numMonitors);
		totalKilled += killed;
		sleep(5); //wait for 5 seconds
	}

	//wait on children to exit, this will block if children dont exit
	//whatever.....
	//for (int i = 0; i < numMonitors; i++){
	// 	int status = 0;
	// 	do{
	// 		wait(&status);
	// 	} while(!WIFEXITED(status)); //keep waiting until we get a child exited
	// 	assert(WEXITSTATUS(status) == EXIT_SUCCESS); //make sure the child exited successfully
	// }


}

int Cleanup(){
	ReportSigintCaughtFile(logPath, totalKilled);
	ReportSigintCaught(stdout, totalKilled);

	//Log metadata and exit
	ReportTotalProcessesKilled(logPath, totalKilled);
	FlushLogger(logPath);
	free(monitors);
	monitors = NULL;
	free(configProcesses);
	configProcesses = NULL;
	free(processesToMonitor);
	processesToMonitor = NULL;
	free(processesNotMonitored);
	processesNotMonitored = NULL;
	exit(EXIT_SUCCESS);
}

//Read through child pipes to get any messages from them. Returns number processes killed.
int ReadThroughChildren(MonitorData* monitors, int numMonitors){
	int numKilled = 0;
	for (int i = 0; i < numMonitors; i++){
		PipeData data = {{0}};
		PipeData* pData = &data;
		int bytesRead;
		bytesRead = read(monitors[i].read_pipe[0], (void*) pData, sizeof(PipeData));
		if (bytesRead < 0 && errno != EAGAIN){ //for non blocking read errno is set to EAGAIN when there is nothing to be read
			fprintf(stderr, "error reading message from child: %s\n", strerror(errno));
			assert(0);
		} if (bytesRead > 0){
			if (bytesRead != sizeof(PipeData)){
				fprintf(stderr, "error reading from child, only partial data read\n");
				//assert(0);
			}
			if (!strcmp(data.type, "KIL")){ //child killed process
				ReportProcessKilled(logPath, &(monitors[i].monitoredProcessData));
				numKilled++;
			}
			memcpy(monitors[i].monitoredProcessData.PID, "0", 1);
		}
	}
	return numKilled;
}

//Either make a new child to monitor or delegate to empty monitor
void DelegateMonitorProcess(ProcessData* processToMonitor, int numProcessesToMonitor, MonitorData* monitors, int* numMonitors){

	for (int i = 0; i < numProcessesToMonitor; i++){
		int childPID = 0;
		int readyMonitorIndex = GetIndexOfReadyMonitor(monitors, *numMonitors);
		if (readyMonitorIndex >= 0){ //process to monitor
			monitors[i].monitoredProcessData = processToMonitor[i];
			PipeData data = {{0}};
			PipeData* pData = &data;
			memcpy(data.type,"NEW",4); //memcpy?
			data.monitoredProcess = processToMonitor[i];
			write(monitors[i].write_pipe[1], pData, sizeof(PipeData));
		} else { //we need to create new monitor
			int read_pipe[2]; //pipe to read from child
			int write_pipe[2]; //pipe to write to child
			if (pipe2(read_pipe, O_NONBLOCK) < 0){ //make read pipe non-blocking
				printf("PIPE ERROR: %s", strerror(errno));
				assert(0);
			} 
			if (!pipe2(write_pipe, 0) < 0){ //want write pipe to be blocking
				assert(0);
				printf("PIPE ERROR: %s", strerror(errno));
			}
			childPID = fork();
			if (childPID >= 0){
				if (childPID ==0){ // child process
					ProcessData processData = processToMonitor[i];
					signal(SIGINT, HangleSigintChild);
					//signal(SIGHUP, SIG_DFL);
					free(monitors);
					monitors = NULL;
					free(processesToMonitor);
					processesToMonitor = NULL;
					free(configProcesses);
					configProcesses = NULL;
					close(write_pipe[1]); //read and write pipe will be reversed for child
					close(read_pipe[0]);
					RunChild(processData, write_pipe ,read_pipe); //this shouldnt return
				} else { //parent process
					close(write_pipe[0]);
					close(read_pipe[1]);
					monitors[i].monitorPID = childPID;
					memcpy(monitors[i].read_pipe, read_pipe, 2 * sizeof(int)); //is this overwriting something?
					memcpy(monitors[i].write_pipe, write_pipe, 2 * sizeof(int));
					monitors[i].monitoredProcessData = processesToMonitor[i]; //TODO(If processToMonitor is freed this will mess up)
				}
			} else {
				assert(0); //unable to fork
			}
			assert(childPID); //child should not reach this point

			ReportMonitoringProcess(logPath, processesToMonitor + i);
			(*numMonitors)++;
		}
	}
}

int GetIndexOfReadyMonitor(MonitorData* monitors, int numMonitors){
	for (int i = 0; i < numMonitors; i++){
		if (monitors[i].monitoredProcessData.PID[0] == '0'){ //this monitor isn't monitoring anything
			return i;
		}
	}
	return -1;
}

void KillOtherProcnanny(){
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
	free(processes);
	processes = NULL;
}

//Get processes that should be monitored. This will read in the processes in the config,
//And then check if these processes are running and if they are already being monitored
ProcessData* GetProcessesToMonitor(char* configLocation, int* numProcesses, MonitorData* monitors, int numMonitors, ConfigData* configProcesses, int numConfigProcesses){

	//Get Running Processes
	int maxNumberOfProcesses = GetMaxNumberOfProcesses();
	ProcessData* processes = (ProcessData*)calloc(maxNumberOfProcesses, sizeof(ProcessData));
	int processesRunning;
	if (GetRunningProcesses(processes, &processesRunning)){
		assert(0);
	}

	//Read In Config File
	// int numProcessesInConfig;
	// ConfigData*  configProcesses = (ConfigData*)malloc(maxNumberOfProcesses * sizeof(ConfigData));
	// GetConfigInfo(configLocation, configProcesses, &numProcessesInConfig);


	//Check What Processes out of config are actually running
	int numProcessesToMonitor;
	//these are processes that are in the config and are running
	ProcessData* processesToMonitor = GetProcessesToTrack(processes, configProcesses, processesRunning, &numProcessesToMonitor);
	LogIfProcessNotRunning(processesToMonitor, configProcesses, numConfigProcesses, numProcessesToMonitor, logPath);

	//need to check which processes are being monitored already
	int numNotMonitored;
	ProcessData* processesNotMonitored  = GetProcessesNotMonitored(processesToMonitor, numProcessesToMonitor, monitors, numMonitors, &numNotMonitored);
	//free(configProcesses); //will memwatch find?
	free(processes);
	processes = NULL;
	free(processesToMonitor);
	processesToMonitor = NULL;
	*numProcesses = numNotMonitored;
	return processesNotMonitored;
}

void HandleSigint(int signal){
	exiting = 1;
	Cleanup();
}

void HangleSigintChild(int signal){
	exiting = 1;
}

void HandleSigHup(int signal){
	reReadConfig++;
}

ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors){
	for (int i = 0; i < numMonitors; i++){
		if (monitors[i].monitorPID == monitorPID) {
			return &(monitors[i].monitoredProcessData);
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

int MonitorProcess(ProcessData process, int* exiting){
	sleep(process.killTime);
	int killSuccess = 0;
	if (!*exiting){
		killSuccess = Kill(process);
	}
	return killSuccess;
}

void RunChild(ProcessData process, int read_pipe[2], int write_pipe[2]){
	PipeData data = {{0}};
	PipeData* pData = &data;
	while (!exiting){
		printf("STARTED CHILD\n");
		int killedError = MonitorProcess(process, &exiting);
		if (!exiting){
			PipeData outData = {{0}};
			PipeData* pOutData = &outData;

			outData.monitoredProcess = process;
			if (!killedError){
				memcpy(outData.type,"KIL",4);
			} else {
				memcpy(outData.type,"TIM",4);
			}
			write(write_pipe[1], (void*) pOutData, sizeof(PipeData));
			read(read_pipe[0], (void*) pData, sizeof(PipeData)); //this should block if the pipe was created as blocking
			printf("DONE READING\n");
			process = data.monitoredProcess;
		}
	}
	exit(EXIT_SUCCESS);
}

//TODO(look in here for bug about processes with same name not being monitored)
ProcessData* GetProcessesNotMonitored(ProcessData* runningProcesses, int numRunningProcesses, MonitorData* monitors, int numMonitors, int* numNotMonitored){
	int alreadyMonitored = 0;
	*numNotMonitored = 0;
	free(processesNotMonitored);
	processesNotMonitored = NULL;
	processesNotMonitored = (ProcessData*) calloc(MAX_PROCESSES, sizeof(ProcessData));\
	for (int i = 0; i < numRunningProcesses; i++){
		alreadyMonitored = 0;
		for (int j = 0; j < numMonitors; j++){
			if (!strcmp(runningProcesses[i].PID, monitors[j].monitoredProcessData.PID)){
				alreadyMonitored = 1;
			}
		}
		if (!alreadyMonitored){
			processesNotMonitored[*numNotMonitored] = runningProcesses[i];
			(*numNotMonitored)++; 
		}
	}
	return processesNotMonitored;
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