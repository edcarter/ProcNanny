#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

char* loggingMode = "w";

//report process from config not running
int ReportProcessNotRunning(char* logLocation, ProcessData* processdata){
	FILE* logFile = fopen(logLocation, loggingMode);
	char dateTimeBuffer[256] = {0};
	GetDateTimeFormat(dateTimeBuffer);
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

//get datetime string
int GetDateTimeFormat(char* buffer){
	time_t currentTime = time(0);
	char* timeFormat = ctime(&currentTime);
	strcpy(buffer, timeFormat);
	return 0;
}