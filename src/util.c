#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// trim function which removes spaces from the start of the commands 
void trim_start(char *str) {
    int idx = 0;
    while (str[idx] == ' ' || str[idx] == '\t' || str[idx] == '\n') {
        idx += 1;
    }

    int idx2 = 0;
    while (str[idx] != '\0') {
        str[idx2] = str[idx];
        idx += 1;
        idx2 += 1;
    }

    str[idx2] = '\0';
}

//trim function which removes spaces from the end 
void trim_end(char *str)
{
    int idx = strlen(str) - 1;
    while (str[idx] == ' ' || str[idx] == '\t' || str[idx] == '\n')
    {
        idx -= 1;
    }
    str[idx + 1] = '\0';
}

void trim(char *str) {
    trim_start(str);
    trim_end(str);
}