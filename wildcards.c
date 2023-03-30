#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef BUFSIZE
#define BUFSIZE 512
#endif

// Recursion function to check if wildcard expression matches fileName
int matchRec(char *exp, char *fileName)
{
    //end of both strings reached, match found
    if (*exp == '\0' && *fileName == '\0')
        return 1;
 
    //fileName does not have ending letters where it should, match not found
    if (*exp == '*' && *(exp + 1) != '\0' && *fileName == '\0') return 0;

    //current letter matches, check next letter
    if (*exp == *fileName) return matchRec(exp + 1, fileName + 1);
 
    // After *, ignore letters until ending matches
    if (*exp == '*') return matchRec(exp + 1, fileName) || matchRec(exp, fileName + 1);
    
    //if none of cases match, either beginning does not match or ending does not match
    return 0;
}

// matchRec initializing function
int match(char *exp, char *fileName)
{
    //do not match hidden files
    if(*exp == '*' && *fileName == '.'){
        return 0;
    }

    return matchRec(exp, fileName);
}

//find files with wildcard
int main(int argc, char **argv)
{
    char *exp = "*"; //testing, this will later be inputted by user
    char *path = "."; //testing, this will later be inputted by user

    DIR *d;
    struct dirent *dir;
    int numFiles = 0; //count number of matching files

    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            //don't match root or child
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {

                //test expression match
                if(match(exp,dir->d_name)){
                    numFiles += 1;
                    printf("%s\n", dir->d_name);
                }
            }
        }
    }

    char **mFileNames; //list of matching file names
    mFileNames = malloc(numFiles * sizeof(char *)); //allocate memory
    
    //put in matching file names
    rewinddir(d);
    int i = 0;
    if (d) {
        
        while ((dir = readdir(d)) != NULL) {
            
            //don't match root or child
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {

                //test expression match
                if(match(exp,dir->d_name)){
                    mFileNames[i] = malloc(255 * sizeof(char)); //255 is the maximum filename size in linux
                    strcpy(mFileNames[i],dir->d_name);
                    i += 1;
                }
            }
        }
        closedir(d);
    }

    //use mFileNames here

    /*
    //print resulting list
    printf("List\n");
    for (i = 0; i<numFiles; i++){
        if(mFileNames[i]!=0){
            printf("%s\n", mFileNames[i]);
        }
    }
    */

    //free mallocs
    for(i=0; i<numFiles; i++){
        free(mFileNames[i]);
    }
    free(mFileNames);

    return EXIT_SUCCESS;
}