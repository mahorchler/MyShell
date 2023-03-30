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
    //changing standard output
    freopen("out.txt","w",stdout);
    printf("hi");

    //changing standard input
    freopen("in.txt","r",stdin);
}