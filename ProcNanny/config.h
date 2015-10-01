#include "processfinder.h"

int GetConfigInfo(char* configLocation, char processNames[128][256], int* numProcesses, int* killTime);
ProcessData* GetProcessesToTrack(ProcessData* processesRunning, char processesInConfig[128][256], int inputCount, int* outputCount);