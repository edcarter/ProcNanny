*NIX process monitor for killing off runaway processes.

Elias Carter
1359203
edcarter

lec section EA1
Paul Lu

lab section ED04
Luke Kukmar


CALLING CONVENTION:
	procnanny <nanny.config location>
	Environment Variable "PROCNANNYLOGS" set to name in existing directory 

BUILD REQUIREMENTS:
	make installed
	gcc installed
	simply run "make" in the command line

ASSUMPTIONS:
	1.) There cannot be more than 128 processes belonging to the user
	2.) Process names cannot be longer than 255 characters

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
 
