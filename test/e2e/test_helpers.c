#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>

int write_results(char *message, char *test, int fd, int nsuccess, int nfail)
{
    // File contains the results of each tests (create, open, write operations)
    int res = write(fd, test, strlen(test));
    if(res == -1){
        sprintf(message, "File write failed\n");
        res = write(fd, message, strlen(message));
        memset(message, 0, 1000);
    }
    sprintf(message, "Success: %d\n", nsuccess);
    res = write(fd, message, strlen(message));
    memset(message, 0, 1000);
    if(res == -1){
        sprintf(message, "File write failed\n");
        res = write(fd, message, strlen(message));
        memset(message, 0, 1000);
    }
    sprintf(message, "Failures: %d\n", nfail);
    res = write(fd, message, strlen(message));
    memset(message, 0, 1000);
    if(res == -1){
        sprintf(message, "File write failed\n");
        res = write(fd, message, strlen(message));
        memset(message, 0, 1000);
    }
    return res;
}