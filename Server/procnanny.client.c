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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <string.h>
#include "processfinder.h"
#include "killer.h"

#define	MY_PORT 6666
#define MAX_PROCESS 128

typedef struct SockData {
	char header[4];
	/* Parent to child messages
	 * CLR: notifies client to clear what processes to monitor
	 * NEW: notifies client of new process to monitor
	 * EXT: notify client to exit
	 */

	/* Child to parent messages
	 * KIL: notify parent that process was killed
	 * TIM: notified parent that a process timed out early
	 */

	ProcessData process;
} SockData;

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

ProcessData* serverprocesses;
ProcessData* runningprocesses;
ProcessData* processestomonitor;
MonitorData* monitors;

int exiting = 0;

void cleanup(int status) {
	free(serverprocesses);
	free(runningProcesses);
	free(processestomonitor);
	free(monitors);

	exit(status);
}

/* returns if there is data in the file to be read */
int readavail(int fd) {
	int maxdesc;
	fd_set read;
	struct timeval tv = {0};

	maxdesc = getdtablesize();
	FD_ZERO(&read);
	FD_SET(fd, &read);

	if (select(maxdesc, &read, NULL, NULL, &tv) < 0)
		perror("procnanny.client: unable to select");

	return FD_ISSET(fd, &read);
}

/* gets the message from the socket */
void getmessage(int s, SockData* message){
	ssize_t justread;
	int totalread = 0; /* number of bytes read in loop and in total*/

	memset(message, 0, sizeof(SockData));

	while (totalread < sizeof(SockData)){
		justread = read(s, (void*) message + totalread, sizeof(SockData) - totalread);
		if (justread < 0){
			perror("procnanny.client: unable to read from server");
			cleanup(EXIT_FAILURE);
			return;
		} else if (justread == 0) {
			cleanup("procnanny.client: unable to read, server closed");
			exit(EXIT_FAILURE);
			return;
		}
		totalread += justread;
	}
}

/* gets processes that are in the config file and are running */
ProcessData* GetProcessesToTrack(ProcessData* runningprocesses, ProcessData* serverprocesses, int inputCount, int* outputCount){
	*outputCount = 0;
	int i,j;
	for (i = 0; i < 128; i++){
		for (j = 0; j < inputCount; j++){
			if (!strcmp(serverprocesses[i].CMD, runningprocesses[j].CMD)){
				(*outputCount)++;
			}
		}
	}

	ProcessData* processesToTrack = (ProcessData*) calloc(*outputCount, sizeof(ProcessData));
	ProcessData* walkingProcessesToTrack = processesToTrack;

	for (i = 0; i < 128; i++){
		for (j = 0; j < inputCount; j++){
			if (!strcmp(serverprocesses[i].CMD, runningprocesses[j].CMD)){
				*walkingProcessesToTrack = runningprocesses[j];
				walkingProcessesToTrack->killTime = serverprocesses[i].killTime;
				walkingProcessesToTrack++;
			}
		}
	}

	return processesToTrack; //make sure to free this
}

//Read through child pipes to get any messages from them. Returns number processes killed.
int ReadThroughChildren(MonitorData* monitors, int numMonitors){
	int i,numKilled = 0;
	for (i = 0; i < numMonitors; i++){
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
				//TODO(need to report to server process was killed)
				//ReportProcessKilled(logPath, &(monitors[i].monitoredProcessData));
				numKilled++;
			}
			memcpy(monitors[i].monitoredProcessData.PID, "0", 1);
		}
	}
	return numKilled;
}

int GetIndexOfReadyMonitor(MonitorData* monitors, int numMonitors){
	int i;
	for (i = 0; i < numMonitors; i++){
		if (monitors[i].monitoredProcessData.PID[0] == '0'){ //this monitor isn't monitoring anything
			return i;
		}
	}
	return -1;
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
			process = data.monitoredProcess;
		}
	}
	exit(EXIT_SUCCESS);
}

//Either make a new child to monitor or delegate to empty monitor
void DelegateMonitorProcess(ProcessData* processToMonitor, int numProcessesToMonitor, MonitorData* monitors, int* numMonitors){
	int i;
	for (i = 0; i < numProcessesToMonitor; i++){
		int childPID = 0;
		int readyMonitorIndex = GetIndexOfReadyMonitor(monitors, *numMonitors);
		if (readyMonitorIndex >= 0){ //process to monitor
			monitors[readyMonitorIndex].monitoredProcessData = processToMonitor[i];
			PipeData data = {{0}};
			PipeData* pData = &data;
			memcpy(data.type,"NEW",4); //memcpy?
			data.monitoredProcess = processToMonitor[i];
			write(monitors[readyMonitorIndex].write_pipe[1], pData, sizeof(PipeData));
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
					free(monitors);
					monitors = NULL;
					free(serverprocesses);
					serverprocesses = NULL;
					free(runningprocesses);
					runningprocesses = NULL;
					free(processestomonitor);
					processestomonitor = NULL;

					close(write_pipe[1]); //read and write pipe will be reversed for child
					close(read_pipe[0]);
					RunChild(processData, write_pipe ,read_pipe); //this shouldnt return
				} else { //parent process
					close(write_pipe[0]);
					close(read_pipe[1]);
					monitors[*numMonitors].monitorPID = childPID;
					memcpy(monitors[*numMonitors].read_pipe, read_pipe, 2 * sizeof(int)); //is this overwriting something?
					memcpy(monitors[*numMonitors].write_pipe, write_pipe, 2 * sizeof(int));
					monitors[*numMonitors].monitoredProcessData = processToMonitor[i]; //TODO(If processToMonitor is freed this will mess up)
				}
			} else {
				assert(0); //unable to fork
			}
			assert(childPID); //child should not reach this point

			(*numMonitors)++;
		}
		/* todo, this needs to go to server */
		//ReportMonitoringProcess(logPath, processesToMonitor + i);
	}
}

void GetProcessesNotMonitored(ProcessData* runningProcesses, int numRunningProcesses, 
								MonitorData* monitors, int numMonitors, 
								ProcessData* processestomonitor, int* n_processestomonitor){
	int i,j,alreadyMonitored = 0;
	*n_processestomonitor = 0;
	//free(processesNotMonitored);
	//processesNotMonitored = NULL;
	//processesNotMonitored = (ProcessData*) calloc(MAX_PROCESSES, sizeof(ProcessData)); //unfreed
	for (i = 0; i < numRunningProcesses; i++){
		alreadyMonitored = 0;
		for (j = 0; j < numMonitors; j++){
			if (!strcmp(runningProcesses[i].PID, monitors[j].monitoredProcessData.PID)){
				alreadyMonitored = 1;
			}
		}
		if (!alreadyMonitored){
			processestomonitor[*n_processestomonitor] = runningProcesses[i];
			(*n_processestomonitor)++; 
		}
	}
}

//TODO(start from here)
ProcessData* GetProcessesToMonitor(MonitorData* monitors, int n_monitors, ProcessData* serverprocesses, 
									int n_serverprocesses, ProcessData* runningprocesses, int n_runningprocesses, int* n_tomonitor){


	//Check What Processes out of config are actually running
	int numProcessesToMonitor;
	//these are processes that are in the config and are running
	ProcessData* processesToMonitor = GetProcessesToTrack(runningprocesses, serverprocesses, 
							n_runningprocesses, &numProcessesToMonitor);

	//LogIfProcessNotRunning(runningprocesses, serverprocesses, 
							//n_serverprocesses, n_runningprocesses, logPath);

	//need to check which processes are being monitored already
	int numNotMonitored;
	GetProcessesNotMonitored(processesToMonitor, numProcessesToMonitor, 
								monitors, n_monitors, processestomonitor, &numNotMonitored);
	*n_tomonitor = numNotMonitored;
	free(processesToMonitor);
	processesToMonitor = NULL;
	return processestomonitor;
}

//Port number of server is passed as second arg
int main(int argc,  char *argv[])
{
	serverprocesses = (ProcessData*) calloc(MAX_PROCESS, sizeof(ProcessData));
	runningprocesses = (ProcessData*) calloc(MAX_PROCESS, sizeof(ProcessData));
	processestomonitor = (ProcessData*) calloc(MAX_PROCESS, sizeof(ProcessData));
	monitors = (MonitorData*) calloc(MAX_PROCESS, sizeof(MonitorData));

	/* socket to server */
	int	s, n_serverprocesses = 0, n_runningprocesses = 0, n_monitors = 0;

	struct	sockaddr_in	server;

	struct	hostent		*host;

	/* last message recieved from server */
	SockData servermessage;



	assert(argc >= 2);

	/* first arg contains server name */
	host = gethostbyname (argv[1]);


	/* stuff to connect to server */
	if (host == NULL) {
		perror ("procnanny.client: cannot get host description");
		exit (1);
	}

	s = socket (AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		perror ("procnanny.client: cannot open socket");
		exit (1);
	}

	bzero (&server, sizeof (server));
	bcopy (host->h_addr, & (server.sin_addr), host->h_length);
	server.sin_family = host->h_addrtype;
	server.sin_port = htons (MY_PORT);

	if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
		perror ("procnanny.client: cannot connect to server");
		exit (1);
	}





	while (1) {

		if (readavail(s)) {
			getmessage(s, &servermessage);
			/* we need to clear out the server processes */
			if(!strcmp(servermessage.header, "CLR")){
				memset(serverprocesses, 0, sizeof(ProcessData) * MAX_PROCESS);
				n_serverprocesses = 0;
			} else if (!strcmp(servermessage.header, "EXT")) { /* we need to exit client */

			} else if (!strcmp(servermessage.header, "NEW")) { /* we need to add a new process to monitor */
				serverprocesses[n_serverprocesses] = servermessage.process;
				n_serverprocesses++;
				GetRunningProcesses(runningprocesses, &n_runningprocesses);
				int n_tomonitor = 0;
				processestomonitor = GetProcessesToMonitor(monitors, n_monitors, serverprocesses, 
									n_serverprocesses, runningprocesses, n_runningprocesses, &n_tomonitor);
				int i;
				for (i = 0; i < n_tomonitor; i++) {
					//serverprocesses[n_serverprocesses] = processestomonitor[i];
					DelegateMonitorProcess(processestomonitor, n_tomonitor, monitors, &n_monitors);
				}
			}
		}

		ReadThroughChildren(monitors, n_monitors);

		/*
		SockData sockdata = {{0}};
		ProcessData process = {{0}};
		strcpy(process.CMD, "CMD FROM CLIENT\n");
		strcpy(process.PID, "1234");
		process.killTime = 10;
		strcpy(sockdata.header, "KIL");
		sockdata.process = process;
		write(s, (void*) &sockdata, sizeof(SockData));
		sleep(5); */

		//sprintf(buf, "Hello There! Here is number %d.\n", number);
		//if(write(s, buf, 128) < 128) {
			//perror("procnanny.client: unable to write to server");
		//}
		//fprintf (stderr, "Sent Number %d\n", number);
		//sleep (5);
		/*i = read(s, buf, sizeof(SockData));
		if (i < 0)
			perror("procnanny.client: unable to read from server");
		if (i != sizeof(SockData))
			break;
		sockdata = (SockData*) buf;
		printf("Recieved SockData: with header=[%s] and process=[%s] and killTime=[%d]\n", 
			sockdata->header, sockdata->process.CMD, sockdata->process.killTime);
		*/

	}
	close (s);
	return EXIT_SUCCESS;
}