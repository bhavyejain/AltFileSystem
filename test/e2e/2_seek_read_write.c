#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "test_helpers.c"

#define NUM_FILES 1000

int main(int argc, char *argv[])
{
    int results_f = open("2_seek_read_write_logs.txt", O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    assert(results_f != -1);

    int fd, nsuccess = 0, nfail = 0;
    int nbytes = -1;
    char message[1000];
    int read_bytes = -1;
    char buffer[100];
    memset(buffer, 0, 100);

    char num[5] = {'\0'};
    char test_file_path[100] = {'\0'};
    char fname[100] = {'\0'};
    char *fname_prefix = "test";
    char *fname_ext = ".txt";

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

        // opening the directory with create mode
        fd = open(test_file_path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
        printf("Creating file: %s\n", test_file_path);
        if(fd == -1)
        {
            printf("error opening the file\n");
            nfail++;
        }
        else
        {
            printf("fd:%d\n",fd);
        }

        // read the first 100 bytes of the file
        read_bytes = read(fd, buffer, 100);
        printf("No of bytes read is %d\n", read_bytes);
        assert(read_bytes == 0);

        lseek(fd, 0, SEEK_SET);
        nbytes = write(fd, test_file_path, strlen(test_file_path));

        memset(buffer, 0, 100);
        lseek(fd, 0, SEEK_SET);
        read_bytes = read(fd, buffer, 100);
        assert(read_bytes == strlen(test_file_path));
        assert(strcmp(test_file_path, buffer) == 0);


        memset(buffer, 0, 100);
        strcpy(buffer, "end");
        lseek(fd, 0, SEEK_END);
        nbytes = write(fd, buffer, strlen(buffer));

        memset(buffer, 0, 100);
        lseek(fd, 0, SEEK_SET);
        read_bytes = read(fd, buffer, 100);
        
        printf("Reading the file after writing end\n No of bytes read is %d\n Content is %s\n", read_bytes, buffer);
        strcat(test_file_path, "end");
        assert(read_bytes == strlen(test_file_path));
        assert(strcmp(test_file_path, buffer) == 0);

        close(fd);
        memset(buffer, 0, 100);
        memset(fname, 0, 100);
        memset(test_file_path, 0, 100);
        nsuccess++;
    }

    memset(message, 0, 1000);
    write_results(message, "Results of SEEK, READ, WRITE:\n",
                  results_f, nsuccess, nfail);
    close(results_f);
    return 0;
}
