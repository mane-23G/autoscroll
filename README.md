# autoscroll
This is a function that autoscrolls a text file passed on by the user. This is tested and created for a Linux System specifically.

###Build with
gcc -o autoscroll autoscroll.c

###Usage 
autoscroll [-s secs] textfile

The user can speficy the rate at which the file should be scrolled. s has to be an interger greater than 0 and greater than 60. If not supplied it is set to 1 second by deafault. 

###Features
-Pause the scrolling by pressing CTRL-Z
-Unpause the scrolling by pressing CTRL-C
-Quit out of the program by pressing CTRL-/ or sending a SIGTERM, SIGQUIT, or SIGKILL
-Displays the line numbers from the text file
-Displays the time

###Defects/Shortcomings
-The clock does not update every second when the autoscroll function is unpaused and the -s option is supplied.
-The program does not use sigwait() and instead uses signal handlers

This was last tested on 9/18/24 and was working as intended (with the stated defects).

Thank you to Professor Weiss for the assigment!
