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

#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <signal.h>
#include "killer.h"
#include "memwatch.h"

int sprintf(char *str, const char *format, ...);
int kill(pid_t pid, int sig);

int Kill(ProcessData process){
	int rv = kill(atoi(process.PID), SIGKILL);
	return rv;
}