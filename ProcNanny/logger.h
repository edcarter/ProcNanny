#include "processfinder.h"

//report process from config not running
int ReportProcessNotRunning(char* logLocation, ProcessData* processdata);

//print monitoring processes
int ReportMonitoringProcess(char* logLocation, ProcessData* processdata);

//print process killed
int ReportProcessKilled(char* logLocation, ProcessData* processdata, int waitTime);

//report total number of processes killed on exit
int ReportTotalProcessesKilled(char* logLocation, int processesKilled);

//flush changes
int FlushLogger(char* logLocation);
