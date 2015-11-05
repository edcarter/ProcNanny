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
int getpid(void);
int Kill(ProcessData process);
int IsOtherProcnanny(ProcessData process);
int sprintf(char *str, const char *format, ...);
void MonitorProcess(ProcessData process);
ProcessData* GetMonitoredProcess(int monitorPID, MonitorData* monitors, int numMonitors);
int LogIfProcessNotRunning(ProcessData* runningProcesses, ConfigData* configProcesses, int numConfigProcesses, int numRunningProcesses, char* logLocation);

int exiting = 0;
int reReadConfig = 1;
ProcessData* processesToMonitor;
MonitorData* monitors; 

int main(int argc, char *argv[]){
	signal(SIGINT, HandleSigint);
	signal(SIGHUP, HandleSighup);
	assert(argc >= 2);
	char* configLocation = argv[1];



	ProcessData* processesToMonitor = GetProcessesToMonitor(configLocation);
	MonitorData* monitors = (MonitorData*) calloc(MAX_PROCESSES, sizeof(MonitorData));
	int numMonitors = 0;

	//main while loop
	while(!exiting){
		if (reReadConfig){
			free (processesToMonitor);
			int numProcessesToMonitor = 0;
			processesToMonitor = GetProcessesToMonitor(configLocation, &numProcessesToMonitor);
			for (int i = 0; i < numProcessesToMonitor; i++){
				numMonitors += DelegateMonitorProcess(processesToMonitor[i], monitors, &numMonitors);
			}
			reReadConfig--;
		}
		int killed = ReadThroughChildren(monitors, numMonitors);
		totalKilled += killed;

		wait(5 * 1000); //wait for 5 seconds
	}

	//Log metadata and exit
	ReportTotalProcessesKilled(logPath, totalKilled);

	FlushLogger(logPath);
	free(monitors);
	free(processesToMonitor);
	free(processes);
	free(configProcesses);
}

//Read through child pipes to get any messages from them. Returns number processes killed.
int ReadThroughChildren(Monitor* monitors, int numMonitors){
	int numKilled = 0;
	for (int i = 0; i < numMonitors; i++){
		PipeData data = {0};
		int read;
		read = read(monitors[i].read_pipe[0], (void*) data, sizeof(PipeData));
		if (read < 0){
			fprintf(stderr, "error reading message from child\n");
			assert(0);
		} if (read > 0){
			if (read != sizeof(PipeData)){
				fprintf(stderr, "error reading from child, only partial data read\n");
				assert(0);
			}
			if (data.type = "KIL"){ //child killed process
				ReportProcessKilled(configLocation, monitors[i].monitoredProcessData);
				numkilled++;
			}
			monitors[i].monitoredProcessData = NULL;
		}
	}
}

//Either make a new child to monitor or delegate to empty monitor
void DelegateMonitorProcess(ProcessData* processToMonitor, int numProcessesToMonitor, MonitorData* monitors, int* numMonitors){
	for (int i = 0; i < numProcessesToMonitor; i++){
		int readyMonitorIndex = GetIndexOfReadyMonitor(monitors, *numMonitors);
		if (readyMonitorIndex){ //process to monitor
			monitor[i].monitoredProcessData = processToMonitor[i];
			PipeData data = {0};
			data.type = "NEW" //memcpy?
			data.monitoredProcess = processToMonitor[i];
			write(monitor[i].write_pipe[1], data, sizeof(PipeData));
		} else { //we need to create new monitor
			int read_pipe[2]; //pipe to read from child
			int write_pipe[2]; //pipe to write to child
			if (!pipe2(read_pipe, O_NONBLOCK)){ //make read pipe non-blocking
				assert(0);
			} 
			if (!pipe2(write_pipe, 0)){ //want write pipe to be blocking
				assert(0);
			}
			childPID = fork();
			if (childPID >= 0){
				if (childPID ==0){ // child process
					free(monitors);
					free(processesToMonitor);
					close(write_pipe[0]); //read and write pipe will be reversed for child
					close(read_pipe[1]);
					RunChild(processToMonitor[i], write_pipe ,read_pipe); //this shouldnt return
				} else { //parent process
					close(read_pipe[1]);
					close(write_pipe[0]);
					monitors[i].monitorPID = childPID;
					monitors[i].read_pipe = read_pipe;
					monitors[i].write_pipe = write_pipe;
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
		if (monitor[i].monitoredProcessData == NULL){ //this monitor isn't monitoring anything
			return i;
		}
	}
	return -1;
}




ProcessData* GetProcessesToMonitor(char* configLocation, int* numProcesses){
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
	free(configProcesses);
	free(processes);
	*numProcesses = numProcessesToMonitor;
	return processesToMonitor;
}

void HandleSigint(int signal){
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

int MonitorProcess(ProcessData process){
	sleep(process.killTime);
	int killSuccess = Kill(process);
}

void RunChild(ProcessData process, int read_pipe[2], int write_pipe[2]){
	PipeData data = {0};
	while (true){	
		int killed = MonitorProcess(process);
		if (killed){
		//need to notify parent when process is killed?
		}
		read(read_pipe[0], data, sizof(PipeData)); //this should block if the pipe was created as blocking
		process = data.monitoredProcess;
	}
	exit(EXIT_SUCCESS);
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







	// //Fork Child Processes for each monitor
	// int childPID = 1;
	// ProcessData processToMonitor;
	// for (int i = 0; i < numProcessesToMonitor; i++){
	// 	monitors[i].monitoredProcessData = &processesToMonitor[i];
	// 	processToMonitor = processesToMonitor[i];
	// 	childPID = fork();
	// 	if (childPID >= 0){
	// 		if (childPID ==0){ // child process
	// 			free(monitors);
	// 			free(processesToMonitor);
	// 			free(processes);
	// 			free(configProcesses);
	// 			RunChild();
	// 		} else { //parent process
	// 			monitors[i].monitorPID = childPID;
	// 			ReportMonitoringProcess(logPath, &processToMonitor);
	// 		}
	// 	} else {
	// 		assert(0); //unable to fork
	// 	}
	// }

	// //child should never get to this point
	// assert(childPID); //childPID == 0 if the child is running this code

	// //Wait for mesages from child processes
	// int totalKilled = 0;
	// int exitStatus = 0;
	// int exitedPID = 0;
	// for (int i = 0; i < numProcessesToMonitor; i++){
	// 	exitedPID = wait(&exitStatus);
	// 	if (exitStatus == EXIT_SUCCESS){
	// 		totalKilled++;
	// 		ProcessData* monitoredProcess = GetMonitoredProcess(exitedPID, monitors, numProcessesToMonitor);
	// 		ReportProcessKilled(logPath, monitoredProcess);
	// 	} else {
	// 		//do nothing, child did not sucessfully kill process
	// 	}
	// }