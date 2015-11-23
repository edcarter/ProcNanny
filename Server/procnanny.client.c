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


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "processfinder.h"

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
			return;
		} else if (justread == 0) {
			perror("procnanny.client: unable to read, server closed");
			return;
		}
		totalread += justread;
	}
}

ProcessData* GetProcessesToTrack(ProcessData* runningprocesses, ProcessData* serverprocesses, int inputCount, int* outputCount){
	*outputCount = 0;

	for (int i = 0; i < 128; i++){
		for (int j = 0; j < inputCount; j++){
			if (!strcmp(serverprocesses[i].CMD, runningprocesses[j].CMD)){
				(*outputCount)++;
			}
		}
	}

	ProcessData* processesToTrack = (ProcessData*) calloc(*outputCount, sizeof(ProcessData));
	ProcessData* walkingProcessesToTrack = processesToTrack;

	for (int i = 0; i < 128; i++){
		for (int j = 0; j < inputCount; j++){
			if (!strcmp(serverprocesses[i].CMD, runningprocesses[j].CMD)){
				*walkingProcessesToTrack = runningprocesses[j];
				walkingProcessesToTrack->killTime = serverprocesses[i].killTime;
				walkingProcessesToTrack++;
			}
		}
	}

	return processesToTrack; //make sure to free this
}

//TODO(start from here)
ProcessData* GetProcessesToMonitor(MonitorData* monitors, int n_monitors, ProcessData* serverprocesses, 
									int n_serverprocesses, ProcessData* runningprocesses, int n_runningprocesses){


	//Check What Processes out of config are actually running
	int numProcessesToMonitor;
	//these are processes that are in the config and are running
	processesToMonitor = GetProcessesToTrack(runningprocesses, serverprocesses, processesRunning, &numProcessesToMonitor);
	LogIfProcessNotRunning(processesToMonitor, configProcesses, numConfigProcesses, numProcessesToMonitor, logPath);

	//need to check which processes are being monitored already
	int numNotMonitored;
	processesNotMonitored  = GetProcessesNotMonitored(processesToMonitor, numProcessesToMonitor, monitors, numMonitors, &numNotMonitored);
	//free(configProcesses); //will memwatch find?
	/*free(processes);
	processes = NULL;
	free(processesToMonitor);
	processesToMonitor = NULL; */
	*numProcesses = numNotMonitored;
	return processesNotMonitored;
}

//Port number of server is passed as second arg
int main(int argc,  char *argv[])
{
	/* socket to server */
	int	s, n_serverprocesses = 0, n_runningprocesses = 0, n_monitors = 0;

	struct	sockaddr_in	server;

	struct	hostent		*host;

	/* last message recieved from server */
	SockData servermessage;

	ProcessData* serverprocesses = (ProcessData*) calloc(MAX_PROCESS, sizeof(ProcessData));
	ProcessData* runningprocesses = (ProcessData*) calloc(MAX_PROCESS, sizeof(ProcessData));
	ProcessData* processesToMonitor = (ProcessData*) calloc(MAX_PROCESS, sizeof(ProcessData));
	MonitorData* monitors = (MonitorData*) calloc(MAX_PROCESSES, sizeof(MonitorData));

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
			if(strcmp(servermessage.header, "CLR")){
				memset(serverprocesses, 0, sizeof(ProcessData) * MAX_PROCESS);
				n_serverprocesses = 0;
			} else if (strcmp(servermessage.header, "EXT")) { /* we need to exit client */

			} else if (strcmp(servermessage.header, "NEW")) { /* we need to add a new process to monitor */
				serverprocesses[n_serverprocesses] = servermessage.process;
				GetRunningProcesses(runningprocesses, n_serverprocesses);
				processesToMonitor = GetPro
				n_serverprocesses++;
			}
		}

		SockData sockdata = {{0}};
		ProcessData process = {{0}};
		strcpy(process.CMD, "CMD FROM CLIENT\n");
		strcpy(process.PID, "1234");
		process.killTime = 10;
		strcpy(sockdata.header, "KIL");
		sockdata.process = process;
		write(s, (void*) &sockdata, sizeof(SockData));
		sleep(5);

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