#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef BUFSIZE
#define BUFSIZE 512
#endif

char *lineBuffer;
int linePos, lineSize, errNum, exitShell;

void append(char *, int);
void dumpLine(void);


int main(int argc, char **argv)
{
    int fin, bytes, pos, lstart;
    char buffer[BUFSIZE];

    // open specified file or read from stdin
    if (argc >= 2) {
        fin = open(argv[0], O_RDONLY);
        if (fin == -1) {
            perror("Unable to open batch file");
            exit(EXIT_FAILURE);
        }
    } else {
        fin = 0;
        errNum = 0;
    }

    // remind user if they are running in interactive mode
    if (isatty(fin)) {
        fputs("Welcome to mysh! Input below:\n", stderr);
	    fputs("mysh> ", stderr);
    }

    // set up storage for the current line
    lineBuffer = malloc(BUFSIZE);
    lineSize = BUFSIZE;
    linePos = 0;

    // read input
    while ((bytes = read(fin, buffer, BUFSIZE)) > 0) {
        if (DEBUG) fprintf(stderr, "read %d bytes\n", bytes);

        // search for newlines
        lstart = 0;
        for (pos = 0; pos < bytes; ++pos) {
            if (buffer[pos] == '\n') {
            int thisLen = pos - lstart + 1;
            if (DEBUG) fprintf(stderr, "finished line %d+%d bytes\n", linePos, thisLen);

            append(buffer + lstart, thisLen);
            dumpLine();
            linePos = 0;
            lstart = pos + 1;
            }
        }
        if (lstart < bytes) {
            // partial line at the end of the buffer
            int thisLen = pos - lstart;
            if (DEBUG) fprintf(stderr, "partial line %d+%d bytes\n", linePos, thisLen);
            append(buffer + lstart, thisLen);
        }
        if (exitShell) {
            return EXIT_SUCCESS;
        }
        if (errNum) {
            fputs("!mysh> ", stderr);
        } else {
            fputs("mysh> ", stderr);
            errNum = 0;
        }
    }

    free(lineBuffer);
    close(fin);

    return EXIT_SUCCESS;
}

// add specified text the line buffer, expanding as necessary
// assumes we are adding at least one byte
void append(char *buf, int len)
{
    int newPos = linePos + len;
    
    if (newPos > lineSize) {
	lineSize *= 2;
	if (DEBUG) fprintf(stderr, "expanding line buffer to %d\n", lineSize);
	assert(lineSize >= newPos);
	lineBuffer = realloc(lineBuffer, lineSize);
	if (lineBuffer == NULL) {
	    perror("line buffer");
	    exit(EXIT_FAILURE);
	}
    }

    memcpy(lineBuffer + linePos, buf, len);
    linePos = newPos;
}

// print the contents of crntLine in standard order
// requires: 
// - linePos is the length of the line in lineBuffer
// - linePos is at least 1
// - final character of current line is '\n'
void dumpLine(void)
{
    char *token;
    char *delim = " \t\n";
    int l = 0, r = linePos - 2;
    char c, cwd [BUFSIZE];
    assert(lineBuffer[linePos-1] == '\n');
    token = strtok(lineBuffer, delim);
    const char *commands[3] = {"cd", "pwd", "exit"};
    char *defaultDirs[6] = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin"};
    DIR *dp;
    struct dirent *de;
    struct stat sfile;
    int commandSize = 3, defaultDirSize = 6;

    // dump output to stdout
    //write(1, lineBuffer, linePos);
   
    while (token != NULL) {
        if (strcmp(token, "exit") == 0) {
            fputs("Terminating Program\n", stderr);
            exitShell = 1; 
            return;
        } else if (strcmp(token, "pwd") == 0) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                fprintf(stderr, "Current working directory: %s\n", cwd);
                errNum = 0;
            } else {
                fputs("getcwd() error\n", stderr);
                errNum = 1;
            }
            return;
        } else if (strcmp(token, "cd") == 0) {
            token = strtok(NULL, delim);
            //change directory
            char *dir = getcwd(cwd, sizeof(cwd));
            strcat(dir, "/");
            strcat(dir, token);
            if (strcmp(token, "..") == 0) {
                if (chdir("..") != 0) {
                    errNum = 1;
                    fprintf(stderr, "failed chdir ..: %s\n", getcwd(cwd, sizeof(cwd)));
                } else {
                    errNum = 0;
                }
            } else if (chdir(dir) != 0) {
                fprintf(stderr, "failed chdir: %s\n", getcwd(cwd, sizeof(cwd)));
                errNum = 1;
            } else {
                fprintf(stderr, "success\n");
                fprintf(stderr, "new dir: %s\n", getcwd(cwd, sizeof(cwd)));
                errNum = 0;
            }
            return;
        } else {
            //search defaultDirs for command
            for (int j = 0; j < defaultDirSize; j++) {
                dp = opendir(defaultDirs[j]);
                if (!dp) {
                    errNum = 1;
                    fprintf(stderr, "Error Opening Directory %s\n", defaultDirs[j]);
                } else {
                    char dir[BUFSIZE];
                    sprintf(dir, "%s/%s", defaultDirs[j], token);
                    if (stat(dir, &sfile) == 0) {
                        //printf("file found %s\n", defaultDirs[j]);
                        int pid = fork();
                        if (pid == 0) {
                            char *args[] = {dir,0};
                            execv(dir,args);
                            
                            // if we got here, something went wrong
                            exit(EXIT_FAILURE);
                        }
                        // we are in the parent process
                        int wstatus;
                        int tpid = wait(&wstatus);   // wait for child to finish
                        if (WIFEXITED(wstatus)) {
                            errNum = 0;
                            break;
                        }
                    } else {
                        errNum = 1;
                    }
                }
            }
            if(errNum){
                fprintf(stderr,"Error: Command Not Found\n");
            }
        }
        token = strtok(NULL, delim);
    } 

    /*for (int i = 0; i < linePos; i++) {
        printf("reach %d", lineBuffer[i]);
    }*/

}