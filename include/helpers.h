#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>

char** split_string(char* str, char* delimeter);
char* get_timestamp();
void writeToLog(char* str, FILE* fp);
void writeToStdOut(char* str);
void set_timer(int duration);
bool rollTheDice(unsigned pct_chance);
unsigned** initializeArray(int m, int n);

#endif
