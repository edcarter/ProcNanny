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
#include <stdio.h>
#include <stdlib.h>

#define	MY_PORT	6666

int mainSock; //main socket for accepting connections

void initMasterSock(){
	struct	sockaddr_in	master;

	mainSock = socket(AF_INET, SOCK_STREAM, 0);
	if (mainSock < 0) {
		perror ("procnanny.server: cannot open master socket");
		exit(EXIT_FAILURE);
	}

	master.sin_family = AF_INET;
	master.sin_addr.s_addr = INADDR_ANY;
	master.sin_port = htons (MY_PORT);

	if (bind (mainSock, (struct sockaddr*) &master, sizeof (master))) {
		perror ("procnanny.server: cannot bind master socket");
		exit (EXIT_FAILURE);
	}
}

int main() 
{


	int	snew, fromlength, number, outnum;

	struct	sockaddr_in from;

	initMasterSock();
	listen (mainSock, 32); //Spec says we will not have more than 32 clients.

	while (1) {
		fromlength = sizeof (from);
		snew = accept (mainSock, (struct sockaddr*) & from, & fromlength);
		if (snew < 0) {
			perror ("Server: accept failed");
			exit (EXIT_FAILURE);
		}
		outnum = htonl (number);
		write (snew, &outnum, sizeof (outnum));
		close (snew);
		number++;
	}
}

