#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test_helpers.c"

#define NUM_FILES 1000

int main(int argc, char *argv[])
{
    int results_f = open("1_create_open_write.txt", O_CREAT | O_RDWR ,  S_IRWXU | S_IRWXG | S_IRWXO);
    assert(results_f != -1);

    int res = -1, nsuccess = 0, nfail = 0;
    int fd;

    char num[5] = {'\0'};
    char fname[100] = {'\0'};
    char *fname_prefix = "test_";
    char *fname_ext = ".txt";
    char test_file_path[100] = {'\0'};
    char message[1000] = {'\0'};

    mkdir("test_1_files", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    for (int i = 1; i < NUM_FILES; i++)
    {
        sprintf(num, "%d", i);  // 1 to 99999
        // Form: test_<num>.txt
        strcat(fname, fname_prefix);
        strcat(fname, num);
        strcat(fname, fname_ext);
        // Form: test_1_files/test_<num>.txt
        strcat(test_file_path, "test_1_files/");
        strcat(test_file_path, fname);

        fd = open(test_file_path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	    printf("Creating file: %s\n", test_file_path);
        if(fd == -1)
        {
            sprintf(message, "%s : Creation failed!\n", fname);
            res = write(results_f, message, strlen(message));
            memset(message, 0, 1000);
        } else
        {
	        printf("Writing to file %s...\n", fname);
            res = write(fd, fname, strlen(fname));
            if(res == -1)
            {
                nfail++;
                sprintf(message, "%s : Write failed!\n", fname);
                res = write(fd, message, strlen(message));
                memset(message, 0, 1000);
            } else
            {
                nsuccess++;
            }
            close(fd);
        }
        memset(fname, 0, 100);
        memset(test_file_path, 0, 100);
    }

    write_results(message, "Results of CREATE, OPEN, WRITE:\n", results_f, nsuccess, nfail);
    close(results_f);
    return 0;
}
