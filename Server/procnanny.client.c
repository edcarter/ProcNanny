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

#define	 MY_PORT  6666

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

//Port number of server is passed as second arg
int main(int argc,  char *argv[])
{
	int	s;

	//char buf[sizeof(SockData) * 2];

	struct	sockaddr_in	server;

	struct	hostent		*host;

	assert(argc >= 2);
	/* Put here the name of the sun on which the server is executed */
	host = gethostbyname (argv[1]);

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
		//read (s, &number, sizeof (number));
		//int i;
		//SockData* sockdata;
		//bzero(buf, sizeof(SockData) * 2);

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