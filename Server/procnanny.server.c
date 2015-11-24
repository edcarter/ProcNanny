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

/*
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 * Some changes (related to the port number) by Paul Lu, March 2011.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * select_server.c - an example of using select to implement a non-forking
 * server. In this case this is an "echo" server - it simply reads
 * input from clients, and echoes it back again to them, one line at
 * a time.
 *
 * to use, cc -DDEBUG -o select_server select_server.c
 * or cc -o select_server select_server.c after you read the code :)
 *
 * then run with select_server PORT
 * where PORT is some numeric port you want to listen on.
 * (Paul Lu:  You can also get the OS to choose the port by not specifying PORT)
 *
 * to connect to it, then use telnet or nc
 * i.e.
 * telnet localhost PORT
 * or
 * nc localhost PORT
 * 
 */


#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "logger.h"
#include "config.h"
#include "memwatch.h"

/* we use this structure to keep track of each connection to us */
struct con {
	int sd; 	/* the socket for this connection */
	int state; 	/* the state of the connection */
	struct sockaddr_in sa; /* the sockaddr of the connection */
	size_t  slen;   /* the sockaddr length of the connection */
	char *buf;	/* a buffer to store the characters read in */
	char *bp;	/* where we are in the buffer */
	size_t bs;	/* total size of the buffer */
	size_t bl;	/* how much we have left to read/write */
	char hostname[128];
};

typedef struct SockData {
	char header[4];
	/* Parent to child messages
	 * CLR: notifies client to clear what processes to monitor
	 * NEW: notifies client of new process to monitor
	 * EXT: notify client to exit
	 */

	/* Child to parent messages
	 * NOT: process does not exist on client
	 * MON: client is now monitoring a process
	 * KIL: notify parent that process was killed
	 * TIM: notified parent that a process timed out early
	 */

	ProcessData process;
} SockData;

/*
 * we will accept a maximum of 256 simultaneous connections to us.
 * While you could make this a dynamically allocated array, and
 * use a variable for maxconn instead of a #define, that is left
 * as an exercise to the reader. The necessity of doing this
 * in the real world is debatable. Even the most monsterous of
 * daemons on real unix machines can typically only deal with several
 * thousand simultaeous connections, due to limitations of the
 * operating system and process limits. so it might not be worth it
 * in general to make this part fully dynamic, depending on your
 * application. For example, there is no point in allowing for
 * more connections than the kernel will allow your process to
 * have open files - so run 'ulimit -a' to see what that is as
 * an example of a typical reasonable number, and bear in mind
 * you may have a few more files open than just your sockets
 * in order to do anything really useful
 */
#define MAXCONN 256
struct con connections[MAXCONN];

int exiting = 0; /*has the server been instructed to exit?*/
int configChanged = 0; /*has the config file been changed?*/

#define BUF_ASIZE 256 /* how much buf will we allocate at a time. */

/* states used in struct con. */
#define STATE_UNUSED 0
#define STATE_ACTIVE 1

#define MYPORT 6666

#define MAXCONFIG 128 /*how many entries can be in the config file. */




/*
 * get a free connection structure, to save a new connection in
 */
struct con * get_free_conn()
{
	int i;
	for (i = 0; i < MAXCONN; i++) {
		if (connections[i].state == STATE_UNUSED){
			struct con* cp = &connections[i];
			cp->state = STATE_ACTIVE;
			cp->bs = sizeof(SockData); /* size of buffer */
			cp->buf = calloc(1, cp->bs); /* allocate enough space for 1 message */
			if (cp->buf == NULL)
				perror("Unable to allocate memory for connection buffer");
			cp->bp = cp->buf; /* location in buffer */
			cp->bl = cp->bs; /* number of bytes to read (size of 1 message) */
			return(cp);
		}
	}
	return(NULL); /* we're all full - indicate this to our caller */
}



/*
 * close or initialize a connection - resets a connection to the default,
 * unused state.
 */
void closecon (struct con *cp, int initflag)
{
	if (!initflag) {
		if (cp->sd != -1)
			close(cp->sd); /* close the socket */
		free(cp->buf); /* free up our buffer */
	}
	memset(cp, 0, sizeof(struct con)); /* zero out the con struct */
	cp->buf = NULL; /* unnecessary because of memset above, but put here
			 * to remind you NULL is 0.
			 */
	cp->sd = -1;
}

/*
 * handlewrite - deal with a connection that we want to write stuff
 * to. assumes the caller has checked that cp->sd is writeable
 * by using select(). once we write everything out, change the
 * state of the connection to the reading state.
 */
void handlewrite(struct con *cp)
{
	ssize_t i;

	/*
	 * assuming before we are called, cp->sd was put into an fd_set
	 * and checked for writeability by select, we know that we can
	 * do one write() and write something. We are *NOT* guaranteed
	 * how much we can write. So while we will be able to write bytes
	 * we don't know if we will get a whole line, or even how much
	 * we will get - so we do *exactly* one write. and keep track
	 * of where we are. If we don't want to block, we can't do
	 * multiple writes to write everything out without calling
	 * select() again between writes.
	 */

	i = write(cp->sd, cp->bp, cp->bl);
	if (i == -1) {
		if (errno != EAGAIN) {
			/* the write failed */
			closecon(cp, 0);
		}
		/*
		 * note if EAGAIN, we just return, and let our caller
		 * decide to call us again when socket is writable
		 */
		return;
	}
	/* otherwise, something ok happened */
	cp->bp += i; /* move where we are */
	cp->bl -= i; /* decrease how much we have left to write */
	if (cp->bl == 0) {
		/* we wrote it all out, hooray, so go back to reading */
		cp->bl = cp->bs; /* we can read up to this much */
		cp->bp = cp->buf;	    /* we'll start at the beginning */
	}
}

/*
 * handleread - deal with a connection that we want to read stuff
 * from. assumes the caller has checked that cp->sd is writeable
 * by using select(). If a newline is seen at the end of what we
 * are reading, change the state of this connection to the writing
 * state. Returns -1 if read in incomplete else the read was complete
 */
int handleread(struct con *cp)
{
	ssize_t i;

	/*
	 * assuming before we are called, cp->sd was put into an fd_set
	 * and checked for readability by select, we know that we can
	 * do one read() and get something. We are *NOT* guaranteed
	 * how much we can get. So while we will be able to read bytes
	 * we don't know if we will get a whole line, or even how much
	 * we will get - so we do *exactly* one read. and keep track
	 * of where we are. If we don't want to block, we can't do
	 * multiple reads to read in a whole line without calling
	 * select() to check for readability between each read.
	 */
	i = read(cp->sd, cp->bp, cp->bl);
	if (i == 0) {
		/* 0 byte read means the connection got closed */
		//closecon(cp, 0);
		//return -1;
	}
	if (i == -1) {
		if (errno != EAGAIN) {
			/* read failed */
			perror("read failed!");
			closecon(cp, 0);
		}
		/*
		 * note if EAGAIN, we just return, and let our caller
		 * decide to call us again when socket is readable
		 */
		return -1;
	}
	/*
	 * ok we really got something read. chage where we're
	 * pointing
	 */
	cp->bp += i;
	cp->bl -= i;

	/*
	 * now check to see if we should change state - i.e. we have
	 * 0 bytes left to read
	 */
	if (cp->bl == 0) {
		cp->bl = sizeof(SockData); /* bytes to read is size of 1 message */
		cp->bp = cp->buf;	   /* and we'll start from here */
		return 1;
	}
	return -1;
}

void sendmessage(struct con *cp, SockData* sockData){
	ssize_t totalsent = 0; /*total bytes sent */
	ssize_t sent = 0; /* bytes sent in one write*/
	while (totalsent < sizeof(SockData)) {
		sent = write(cp->sd, (void*) sockData + totalsent, sizeof(SockData) - totalsent);
		if (sent <= 0){
			char errormsg[128];
			sprintf(errormsg, "error sending message to %s", inet_ntoa(cp->sa.sin_addr));
			perror(errormsg);
			closecon(cp, 0);
		}
		totalsent += sent;
	}
}

/* reset the clients so that they stop monitoring the processes
 * that were sent before. This is done when a new config file
 * is read on the server */
void resetclients() {
	int i;
	struct con connection = {0};
	SockData resetmessage = {{0}};
	strcpy(resetmessage.header,"CLR");
	for (i = 0; i < MAXCONN; i++){
		connection = connections[i]; 
		if (connection.state != STATE_UNUSED){
			sendmessage(&connection, &resetmessage);
		}
	}
}

/* convert a config to a message telling the client to start
 * monitoring the process in the config */
void configtomessage(ConfigData config, SockData * message){
	strcpy(message->header, "NEW");
	ProcessData process = {{0}};
	strcpy(process.CMD, config.CMD);
	process.killTime = config.killTime;
	message->process = process;
}

/* update a client with new processes to be monitored */
void updateclient(struct con* connection, ConfigData* configs, int numConfigs){
	int j;
	SockData message = {{0}};
	for (j = 0; j < numConfigs; j++){
		configtomessage(configs[j], &message); /*fill in the message using the config*/
		sendmessage(connection, &message);
	}
}

/* update the clients to monitor new processes
 * by sending them the processes that were recently 
 * read in */
void updateclients(ConfigData* configs, int numConfigs) {
	int i;
	struct con connection = {0};
	for (i = 0; i < MAXCONN; i++){
		connection = connections[i];
		if (connection.state != STATE_UNUSED){
			updateclient(&connection, configs, numConfigs);
		}
	}
}

/* update the clients to monitor new processes
 * by sending them the processes that were recently 
 * read in */
void closeclients() {
	int i;
	struct con connection = {0};
	SockData message = {{0}};
	for (i = 0; i < MAXCONN; i++){
		connection = connections[i];
		if (connection.state != STATE_UNUSED){
			strcpy(message.header, "EXT");
			sendmessage(&connection, &message);
			closecon(&connection, 0);
		}
	}
}





void HandleSigHup(int signal){
	configChanged = 1;
}

void HandleSigint(int signal){
	exiting = 1;
}



int main(int argc,  char *argv[])
{
	signal(SIGINT, HandleSigint);
	signal(SIGHUP, HandleSigHup);

	struct sockaddr_in sockname;
	int max = -1, omax;	     /* the biggest value sd. for select */
	int sd;			     /* our listen socket */
	fd_set *readable = NULL; /* fd_sets for select */
	int i, numkilled = 0;

	int s_hostname = 128;
	char hostname[s_hostname];
	memset(hostname, 0, s_hostname);

	assert(argc >= 2);
	char* config_location = argv[1];


	if (gethostname(hostname, s_hostname) == -1)
		perror("procnanny.server: unable to get hostname");

	int host_pid = getpid();

	char* serv_info = getenv("PROCNANNYSERVERINFO");

	if (serv_info == NULL)
		perror("procnanny.server: failure to get env var PROCNANNYSERVERINFO");

	char* nanny_log = getenv("PROCNANNYLOGS");

	if (nanny_log == NULL)
		perror("procnanny.server: failure to get env var PROCNANNYLOGS");

	ReportServerStarted(stdout, host_pid, hostname, MYPORT);
	ReportServerStartedFile(nanny_log, host_pid, hostname, MYPORT);
	ReportServerStartedFile2(serv_info, host_pid, hostname, MYPORT);
	FlushLogger(nanny_log);
	FlushLogger(serv_info);
	fflush(stdout);

	int numConfigs = 0;
	ConfigData* configs = (ConfigData*) calloc(MAXCONFIG, sizeof(ConfigData));
	GetConfigInfo(config_location, configs, &numConfigs); /*populate configs from the config file*/


	/* now off to the races - let's set up our listening socket */
	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(MYPORT);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd= socket(AF_INET,SOCK_STREAM,0);
	if ( sd == -1)
		perror("procnanny.server: main socket failed");

	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		perror("procnanny.server: main socket bind failed");

	if (listen(sd,32) == -1)
		perror("procnanny.server: main socket listen failed");

	/* 
	 * We're now bound, and listening for connections on "sd".
	 * Each call to "accept" will return us a descriptor talking to
	 * a connected client.
	 */

	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
	/* initialize all our connection structures */
	for (i = 0; i < MAXCONN; i++)
		closecon(&connections[i], 1);

	while(!exiting) {

		if (configChanged){
			memset(configs, 0, sizeof(ConfigData) * MAXCONFIG); /* clear configs */
			GetConfigInfo(config_location, configs, &numConfigs); /* read in new configs */
			/* need to notify clients of config change (CLR) and send processes to monitor (NEW) */
			resetclients();
			updateclients(configs, numConfigs);
			configChanged = 0;
		}

		int i;
		int maxfd = -1; /* the biggest value sd we are interested in.*/

		/*
		 * first we have to initialize the fd_sets to keep
		 * track of readable and writable sockets. we have
		 * to make sure we have fd_sets that are big enough
		 * to hold our largest valued socket descriptor.
		 * so first, we find the max value by iterating through
		 * all the connections, and then we allocate fd sets
		 * that are big enough, if they aren't already.
		 */
		omax = max;
		max = sd; /* the listen socket */

		for (i = 0; i < MAXCONN; i++) {
			if (connections[i].sd > max)
				max = connections[i].sd;
		}
		if (max > omax) {
			/* we need bigger fd_sets allocated */

			/* free the old ones - does nothing if they are NULL */
			free(readable);

			/*
			 * this is how to allocate fd_sets for select
			 */
			readable = (fd_set *)calloc(howmany(max + 1, NFDBITS),
			    sizeof(fd_mask));
			if (readable == NULL)
				perror("procnanny.server: cannot allocate memory for fd_set");
			omax = max;
			/*
			 * note that calloc always returns 0'ed memory,
			 * (unlike malloc) so these sets are all set to 0
			 * and ready to go
			 */
		} else {
			/*
			 * our allocated sets are big enough, just make
			 * sure they are cleared to 0. 
			 */
			memset(readable, 0, howmany(max+1, NFDBITS) *
			    sizeof(fd_mask));
		}

		/*
		 * Now, we decide which sockets we are interested
		 * in reading and writing, by setting the corresponding
		 * bit in the readable and writable fd_sets.
		 */

		/*
		 * we are always interesting in reading from the
		 * listening socket. so put it in the read set.
		 */

		FD_SET(sd, readable);
		if (maxfd < sd)
			maxfd = sd;

		/*
		 * now go through the list of connections, and if we
		 * are interested in reading from, or writing to, the
		 * connection's socket, put it in the readable, or
		 * writable fd_set - in preparation to call select
		 * to tell us which ones we can read and write to.
		 */
		for (i = 0; i<MAXCONN; i++) {
			if (connections[i].state == STATE_ACTIVE) {
				FD_SET(connections[i].sd, readable);
				if (maxfd < connections[i].sd)
					maxfd = connections[i].sd;
			}
		}

		/*
		 * finally, we can call select. we have filled in "readable"
		 * and "writable" with everything we are interested in, and
		 * when select returns, it will indicate in each fd_set
		 * which sockets are readable and writable
		 */
		struct timeval timeout = {0};
		i = select(maxfd + 1, readable, NULL, NULL, &timeout);
		if (i == -1  && errno != EINTR)
			perror("procnanny.server: select failed");
		if (i > 0) {

			/* something is readable or writable... */

			/*
			 * First things first.  check the listen socket.
			 * If it was readable - we have a new connection
			 * to accept.
			 */

			if (FD_ISSET(sd, readable)) {
				struct con *cp;
				int newsd;
				socklen_t slen;
				struct sockaddr_in sa;

				slen = sizeof(sa);
				newsd = accept(sd, (struct sockaddr *)&sa,
				    &slen);
				if (newsd == -1)
					perror("procnanny.server: accept failed");

				cp = get_free_conn();
				if (cp == NULL) {
					/*
					 * we have no connection structures
					 * so we close connection to our
					 * client to not leave him hanging
					 * because we are too busy to
					 * service his request
					 */
					close(newsd);
				} else {
					/*
					 * ok, if this worked, we now have a
					 * new connection. set him up to be
					 * READING so we do something with him
					 */
					cp->sd = newsd;
					cp->slen = slen;

					char from[128];
					memset(from, 0, 128);
					memcpy(&cp->sa, &sa, sizeof(sa));
					if (!getnameinfo((struct sockaddr*) &cp->sa, (socklen_t) cp->slen, from, 128, NULL, 0, 0))
							perror("procnanny.server, unable to resolve client host name");
					memcpy(cp->hostname, from, 128);

					updateclient(cp, configs, numConfigs);
				}
			}
			/*
			 * now, iterate through all of our connections,
			 * check to see if they are readble or writable,
			 * and if so, do a read or write accordingly 
			 */
			for (i = 0; i<MAXCONN; i++) {
				if ((connections[i].state != STATE_UNUSED) &&
				    FD_ISSET(connections[i].sd, readable))
				{
					if (handleread(&connections[i]) > 0){
						/*char from[128];
						memset(from, 0, 128);
						*/
						struct con cp = connections[i]; 
						SockData* sockdata = (SockData*) cp.buf;

						/* get host name from our connection */
						/*if (!getnameinfo((struct sockaddr*) &cp.sa, (socklen_t) cp.slen, from, 128, NULL, NULL, NULL))
							perror("procnanny.server, unable to resolve client host name"); */

						/* we need to log processes that are killed / timed out */
						printf("Recieved SockData from %s: with header=[%s] and process=[%s] and killTime=[%d]\n", cp.hostname,
							sockdata->header, sockdata->process.CMD, sockdata->process.killTime);

						if (!strcmp(sockdata->header, "NOT")) {
							//process does not exist on client
							ReportProcessNotRunning(nanny_log, &sockdata->process, cp.hostname);
						} else if (!strcmp(sockdata->header, "MON")) {
							//process is being monitored
							ReportMonitoringProcess(nanny_log, &sockdata->process, cp.hostname);
						} else if (!strcmp(sockdata->header, "KIL")) {
							//process was killed
							ReportProcessKilled(nanny_log, &sockdata->process, cp.hostname);
							numkilled++;
						} else if (!strcmp(sockdata->header, "TIM")) {
							//process timed out early, don't need to log
						}
					}

				} /* if (connection can be read) */

			} /* for */

		} /* if (i > 0)... i = select() */

	} /* while(!exiting) */

	for (i = 0; i < MAXCONN; i++){
		if (connections[i].state != STATE_UNUSED) {

		}
	}
	
	FlushLogger(nanny_log);
	closeclients();
	close(sd);



    /* here we need to notify the clients to close 
     * and clean up any resources that the server uses */
	return EXIT_SUCCESS;
} /* main */