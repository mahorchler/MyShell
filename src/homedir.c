#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>

#ifndef DEBUG
#define DEBUG 0
#endif

int main(int argc, char **argv)
{
    char *homeDir = getenv("HOME");
    printf("%s\n",homeDir);
}