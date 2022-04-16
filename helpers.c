#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "helpers.h"

char** split_string(char* str, char* delimeter)
{
    char** strings = malloc(10 * sizeof(char*));
    char* substr;

    substr = strtok(str, delimeter);

    int i = 0;
    while (substr != NULL)
    {
        strings[i] = substr;
        substr = strtok(NULL, delimeter);
        i++;
    }

    return strings;

}

char* get_timestamp()
{
    char* timestamp = malloc(sizeof(char)*10);
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(timestamp, "%d:%d:%d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    return timestamp;
}

void set_timer(int duration) {
    struct itimerval value;
    value.it_interval.tv_sec = duration;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    if (setitimer(ITIMER_REAL, &value, NULL) == -1) {
        perror("setitimer");
        exit(1);
    }
}

void writeToLog(char* str, FILE* fp)
{
    fputs(str, fp);
}

void writeToStdOut(char* str)
{
    fputs(str, stdout);
}

bool rollTheDice(unsigned pct_chance) {
    unsigned percent = (rand() % 100) + 1;

    if (percent <= pct_chance) return true;
    else return false;
}

unsigned** initializeArray(int m, int n) {
    unsigned* values = calloc(m*n, sizeof(unsigned));
    unsigned** rows = malloc(n*sizeof(unsigned*));
    for (int i = 0; i < n; ++i)
    {
        rows[i] = values + i*m;
    }
    return rows;
}
