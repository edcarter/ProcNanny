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

#include "processfinder.h"

//report process from config not running
int ReportProcessNotRunning(char* logLocation, ProcessData* processdata);

//print monitoring processes
int ReportMonitoringProcess(char* logLocation, ProcessData* processdata);

//print process killed
int ReportProcessKilled(char* logLocation, ProcessData* processdata);

//report total number of processes killed on exit
int ReportTotalProcessesKilled(char* logLocation, int processesKilled);

void ReportParentPid(FILE* logFile, int parentPid);

int ReportParentPidFile(char* logLocation, int parentPid);

void ReportSighupCaught(FILE* logFile, char* configLocation);

int ReportSighupCaughtFile(char* logLocation, char* configLocation);

void ReportSigintCaught(FILE* location, int processesKilled);

int ReportSigintCaughtFile(char* logLocation, int processesKilled);

int FlushLogger(char* logLocation);