#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

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
	fprintf(logFile, "[%s] Info: No \'%s\' processes found.", dateTimeBuffer, processNameBuffer);
	return fclose(logFile);
}

//print monitoring processes
int ReportMonitoringProcess(char* logLocation, ProcessData* processdata){

}

//print process killed
int ReportProcessKilled(char* logLocation, ProcessData* processdata, int waitTime){

}

//report total number of processes killed on exit
int ReportTotalProcessesKilled(char* logLocation, int processesKilled){

}

//flush changes
int FlushLogger(char* logLocation){
	FILE* logFile = fopen(logLocation, loggingMode);
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