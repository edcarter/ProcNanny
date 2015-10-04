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

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"
#include "memwatch.h"

char* loggingMode = "a";

FILE* OpenFile(char* path);
int GetDateTimeFormat(char* buffer);

//report process from config not running
int ReportProcessNotRunning(char* logLocation, ProcessData* processdata){
	FILE* logFile = OpenFile(logLocation);
	char dateTimeBuffer[256] = {0};
	GetDateTimeFormat(dateTimeBuffer);
	strtok(dateTimeBuffer, "\n");
	char processNameBuffer[256] = {0};
	strcpy(processNameBuffer, processdata->CMD);
	strtok(processNameBuffer, "\n");
	fprintf(logFile, "[%s] Info: No \'%s\' processes found.\n", dateTimeBuffer, processNameBuffer);
	return fclose(logFile);
}

//print monitoring processes
int ReportMonitoringProcess(char* logLocation, ProcessData* processdata){
	FILE* logFile = OpenFile(logLocation);
	char dateTimeBuffer[256] = {0};
	GetDateTimeFormat(dateTimeBuffer);
	strtok(dateTimeBuffer, "\n");
	char processNameBuffer[256] = {0};
	strcpy(processNameBuffer, processdata->CMD);
	strtok(processNameBuffer, "\n");
	fprintf(logFile, "[%s] Info: Initializing monitoring of process \'%s\' (PID %s).\n", dateTimeBuffer, processNameBuffer, processdata->PID);
	return fclose(logFile);
}

//print process killed
int ReportProcessKilled(char* logLocation, ProcessData* processdata, int waitTime){
	FILE* logFile = OpenFile(logLocation);
	char dateTimeBuffer[256] = {0};
	GetDateTimeFormat(dateTimeBuffer);
	strtok(dateTimeBuffer, "\n");
	char processNameBuffer[256] = {0};
	strcpy(processNameBuffer, processdata->CMD);
	strtok(processNameBuffer, "\n");
	fprintf(logFile, "[%s] Action: PID %s (\'%s\') killed after exceeding %d seconds.\n", dateTimeBuffer, processdata->PID, processNameBuffer, waitTime);
	return fclose(logFile);
}

//report total number of processes killed on exit
int ReportTotalProcessesKilled(char* logLocation, int processesKilled){
	FILE* logFile = OpenFile(logLocation);
	char dateTimeBuffer[256] = {0};
	GetDateTimeFormat(dateTimeBuffer);
	strtok(dateTimeBuffer, "\n");
	fprintf(logFile, "[%s] Info: Exiting. %d process(es) killed.\n\n", dateTimeBuffer, processesKilled);
	return fclose(logFile);
}

//flush changes
int FlushLogger(char* logLocation){
	FILE* logFile = OpenFile(logLocation);
	int success = fflush(logFile);
	return fclose(logFile) && success;
}

int ConvertHomePathToAbsolute(char* relPath, char* absPath){
	char* homePath = getenv("HOME");
	char relPathBuffer[256] = {0};
	char * pBuffer = relPathBuffer;
	pBuffer++; //dont want ~ at beginning so skip over it
	strcpy(relPathBuffer, relPath);
	strcat(absPath, homePath);
	strcat(absPath, pBuffer);
	return 0;
}

FILE* OpenFile(char* path){
	FILE* file;
	if (path[0] == 126){ //~ character
		char absPath[256] = {0};
		ConvertHomePathToAbsolute(path, absPath);
		file = fopen(absPath, loggingMode);
	} else {
		file = fopen(path, loggingMode);
	}
	return file;
}

//get datetime string
int GetDateTimeFormat(char* buffer){
	time_t currentTime = time(0);
	char* timeFormat = ctime(&currentTime);
	strcpy(buffer, timeFormat);
	return 0;
}