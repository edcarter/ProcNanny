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

#define	 MY_PORT  6666

//Port number of server is passed as second arg
int main(int argc,  char *argv[])
{
	int	s, number = 0;

	char buf[128];

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
		bzero(buf, 128);
		sprintf(buf, "Hello There! Here is number %d.\n", number);
		if(write(s, buf, 128) < 128) {
			perror("procnanny.client: unable to write to server");
		}
		fprintf (stderr, "Sent Number %d\n", number);
		sleep (5);
	}
	close (s);

}