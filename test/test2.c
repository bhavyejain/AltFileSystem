#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<stdio.h>

// Simple program to open file and write to it
// Variants to test:
// ./test2 ~/<mount folder>/hi.txt "This is a string"
// ~/<folder where test resides>/test2 ./hi.txt "This is a string" (cd into the mount folder and run this)
int main(int argc, char *argv[])
{
        if (argc != 3)
        {
                printf("Some parameters are missing\n. Run as <programname> filename string to write\n");
                return 0;
        }

        char filepath[256], text[4096];
        strcpy(filepath, argv[1]);
        strcpy(text, argv[2]);

        int fptr;
        fptr = open(filepath, O_CREAT | O_RDWR, 0666);

        if (fptr < 0)
        {
                printf("Error! FD: %d\n", fptr);
                exit(1);
        }

        printf("File descriptor: %d\n", fptr);

        int byteswritten = write(fptr, text, sizeof(text));

        printf("Wrote %d bytes to file %s\n",byteswritten, filepath);

        return 0;
}