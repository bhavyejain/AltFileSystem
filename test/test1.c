#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int fptr;

    printf("Trying to open a file...\n");
    fptr = open("mnt/hi.txt", O_CREAT | O_RDWR, 0666);
    printf("Checking the file descriptor...\n");

    if(fptr < 0)
    {
        printf("Error! FD: %d\n", fptr);
        exit(1);
    }

    printf("File descriptor: %d\n", fptr);
    printf("\n");

    char *buf = "haha";
    printf("Trying to write a string [%s] to the file.\n", buf);
    int written = write(fptr, buf, 4);
    printf("Bytes written: %d\n", written);
    printf("\n");

    char *read_buffer[50];
    printf("Reading without lseek()...\n");
    read(fptr, read_buffer, 4);
    printf("Read: [%s]\n", read_buffer);
    printf("\n");

    memset(read_buffer, 0, sizeof(read_buffer));
    printf("Doing lseek() to byte 0\n");
    lseek(fptr, 0, SEEK_SET);
    printf("Reading after lseek()...\n");
    read(fptr, read_buffer, 4);
    printf("Read: [%s]\n", read_buffer);
    printf("\n");

    printf("Closing file descriptor...\n");
    close(fptr);

   return 0;
}