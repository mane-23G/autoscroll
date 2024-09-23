/*
Title: autoscroll.c
Author: Mane Galstyan
Created on: May 13, 2024
Description: autoscroll a file
Usage: autoscroll [-s secs] textfile
Build with: gcc -o autoscroll autoscroll.c
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>

/* global variables*/
char* filebuffer;
int* location;
int* total;
int* newline;
int currentline = 1;
int totallines = 0;
int actualline = 1;
int acctuallast = 1;
unsigned short int rows, cols;
volatile sig_atomic_t paused = 0;
sigset_t alarm_mask;
int delay = 1;
int printedlines = 0;

//usage [-s secs] textfile


/* prints error*/
void fatal_error(int errornum, const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

/* prints usage error*/
void usage_error(const char *msg) {
    printf("%s\n",msg);
    exit(EXIT_FAILURE);
}

/* get window size*/
void winsize(int fd, unsigned short int *row, unsigned short int *col) {
    struct winsize w;
    if(ioctl(fd, TIOCGWINSZ, &w) == -1) {
        fatal_error(errno, "Error getting windowsize");
    }
    *row = w.ws_row;
    *col = w.ws_col;
}

/* clear screen*/
void clearscrren() {
    printf("\033[%d;1H",rows);
    printf("\033[2J");
    printf("\033[3J");
    printf("\033[1;1H");
    fflush(stdout);
    
}

/* move cursor back after input or signal is recieved*/
void moveback() {
    printf("\033[%d;%dH",rows,cols-2);
    fflush(stdout);
}

/* display time*/
void displaytime() {
    time_t now;
    struct tm *nowtm;
    char buf[64];
    
    now = time(NULL);
    nowtm = localtime(&now);
    
    printf("\033[%d;%dH",rows,1);
    fflush(stdout);
    strftime(buf, sizeof(buf), "%H:%M:%S", nowtm);
    printf("%s Lines: %d-%d", buf,actualline,acctuallast);
    printf("\033[%d;%dH",rows,cols-2);
    fflush(stdout);

}

/* print the lines from the buffer*/
void printlines() {
    printf("\033[%d;%dH\033[1J\033[1;1H",rows-1,cols);
    fflush(stdout);
    if(currentline >= totallines) {
        clearscrren();
        exit(EXIT_SUCCESS);
    }
    char line[cols+1];
    int rowsprinted = 0;
    printedlines = currentline;
    if(newline[printedlines] > 0) {
        actualline = newline[printedlines];
    }
    while(rowsprinted < rows-1 && printedlines < totallines) {
        memset(line, '\0', cols+1);
        strncpy(line, filebuffer + total[printedlines], location[printedlines]);
        line[cols] = '\0';
        printf("%s\n",line);
        fflush(stdout);
        rowsprinted++;
        if(newline[printedlines] > 0) acctuallast = newline[printedlines];
        printedlines++;
    }
    displaytime();
    fflush(stdout);
    currentline++;
}

/* The signal handler for SIGALRM */
void catchalarm(int signum) {
    alarm(delay);
    printlines();
}

/* The signal handler for SIGTSTP */
void pausescroll(int signum) {
    moveback();
    sigprocmask(SIG_BLOCK, &alarm_mask, NULL); // Block SIGALRM
    paused = 1;
    while(paused) {
        displaytime();
        sleep(1);
    }
}

/* The signal handler for SIGINT */
void unpausescroll(int signum) {
    paused = 0;
    moveback();
    sigprocmask(SIG_UNBLOCK, &alarm_mask, NULL); // Unblock SIGALRM
}

/* The signal handler for SIGTERM and SIGQUIT */
void end(int signum) {
    clearscrren();
    exit(EXIT_SUCCESS);
}

int main( int argc, char *argv[]) {
    char msg[512];
    if(argc < 2) {
        sprintf(msg,"%s [-s secs] textfile",argv[0]);
        usage_error(msg);
    }
    
    
    /* parse for option s*/
    int s = 1;
    int s_option = 0;
    char* s_arg;
    
    while((s = getopt(argc, argv, "s:")) != -1) {
        switch(s) {
            case 's':
                delay = strtol(optarg, NULL, 10);
                if(errno == ERANGE || delay < 1 || delay > 59) {
                    printf("%s [-s secs] textfile\n",argv[0]);
                    fatal_error(EXIT_FAILURE, "0 < secs < 60");
                }
                break;
            case ':':
                sprintf(msg, "%s [-s secs] textfile", argv[0]);
                usage_error(msg);
            case '?':
                sprintf(msg, "%s [-s secs] textfile", argv[0]);
                usage_error(msg);
        }
    }
    
    /* get window row and col size*/
    if (isatty(STDIN_FILENO) == 0) {
        fatal_error(errno, "Not a terminal");
    }
    winsize(STDIN_FILENO, &rows, &cols);
        
    /* open and read textfile*/
    char* filename = argv[optind];
    
    /* open file*/
    int fd = open(filename,O_RDONLY);
    if(fd == -1) fatal_error(errno, "Unable to open file");
    
    /* get file size*/
    intmax_t filesize = lseek(fd, 0, SEEK_END);
    if(filesize == -1) fatal_error(errno, "Unable to seek file");
    
    /* allocate memmory for buffer*/
    filebuffer = malloc(filesize);
    if(!filebuffer) fatal_error(errno, "Unable to allocate memmory");
    
    /* copy file into buffer*/
    if(lseek(fd, 0, SEEK_SET) == (off_t)-1)
        fatal_error(errno, "Unable to seek file");
    if(read(fd, filebuffer, filesize) != filesize)
        fatal_error(errno, "Error reading from file");
    
    /* close file*/
    close(fd);
    
    /* record all new line amount*/
    location = malloc(filesize * sizeof(int));
    if (!location) fatal_error(errno, "Unable to allocate memory for location array");

    /* record all new line displacment*/
    total = malloc(filesize * sizeof(int));
    if (!total) fatal_error(errno, "Unable to allocate memory for total array");

    /* record all new lines*/
    newline = malloc(filesize * sizeof(int));
    if (!total) fatal_error(errno, "Unable to allocate memory for total array");
    memset(newline, 0, filesize);
    

    /* parse file for new line locations and record the offset*/
    location[0] = 0;
    total[0] = 0;
    totallines = 1;
    int start = 0;
    int line = 1;
    int length = 0;
    char pine[cols+1];
    for(int i = 0; i < filesize; i++) {
        if(filebuffer[i] == '\n') {
            newline[totallines] = line;
            line++;
            length = i - start;
            while(length > cols) {
                location[totallines] = cols;
                total[totallines] = start;
                start += location[totallines];
                totallines++;
                length -= cols;
            }
            if(length > 0) {
                location[totallines] = length;
                total[totallines] = start;
                start += location[totallines] + 1;
                totallines++;
                length -= cols;
            }
        }
    }
    
    /* signal mask to block SIGALRM*/
    sigemptyset(&alarm_mask);
    sigaddset(&alarm_mask, SIGALRM);
    
    /* signal alarm*/
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = catchalarm;
    sigemptyset(&(act.sa_mask));
    sigaction(SIGALRM, &act, NULL);
    
    
    /* signal z*/
    struct sigaction act1;
    memset(&act1, 0, sizeof(act1));

    act1.sa_handler = pausescroll;
    sigemptyset(&(act1.sa_mask));
    sigaction(SIGTSTP, &act1, NULL);
    
    /* signal c*/
    struct sigaction act2;
    memset(&act2, 0, sizeof(act2));

    act2.sa_handler = unpausescroll;
    sigemptyset(&(act2.sa_mask));
    sigaction(SIGINT, &act2, NULL);
    
    /* signal exit*/
    struct sigaction act3;
    memset(&act3, 0, sizeof(act3));

    act3.sa_handler = end;
    sigemptyset(&(act3.sa_mask));
    sigaction(SIGQUIT, &act3, NULL);
    sigaction(SIGTERM, &act3, NULL);
    
    /* if lines in file is less than rows-1 display lines then exit*/
    if(totallines - 1<= rows - 1) {
        printlines();
        sleep(delay);
        clearscrren();
        exit(EXIT_SUCCESS);
    }
    
    /* start alarm and print the first rows*/
    alarm(delay);
    clearscrren();
    printlines();
    while(1)
       pause();
}

