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
        memset(lineBuffer, 0, BUFSIZE); //reset lineBuffer
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
        if (fin == 0) {
            if (errNum) {
                fputs("!mysh> ", stderr);
            } else {
                fputs("mysh> ", stderr);
                errNum = 0;
            }
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

//execute child process
void executeLine(char *dir, char *token){
    char *delim = " \t\n";
    char *lineBufferCopy = malloc(BUFSIZE);

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe error");
        errNum = 1;
        return;
    }
    int pid = fork();
    if (pid == 0) {
        //set up pipe
        close(pipe_fd[0]); //close read end of pipe
        dup2(pipe_fd[1], STDERR_FILENO); //redirect stderr to write end of pipe
        close(pipe_fd[1]);
        //set up arguments
        int argCount = 0; //count arguments
        token = strtok(NULL, delim);
        while(token != NULL){
            argCount+=1;
            token = strtok(NULL, delim);
        }

        char **args = malloc((argCount+2) * sizeof(char *)); //allocate argument space
        args[0] = (char *) malloc(strlen(dir)*sizeof(char));
        strcpy(args[0],dir); //put dir as first argument

        //add arguments to args
        strcpy(lineBufferCopy,lineBuffer);
        token = strtok(lineBufferCopy,delim); //reset token
        token = strtok(NULL, delim);
        int i = 1;
        while(token != NULL){
            args[i] = (char *) malloc(strlen(token)*sizeof(char));
            strcpy(args[i],token);
            token = strtok(NULL, delim);
            i+=1;
        }
        args[i]=NULL; //make last argument null

        if(execv(dir,args)){
            errNum=1;
        };
        
        // if we got here, something went wrong
        exit(EXIT_FAILURE);
    }
    // we are in the parent process
    close(pipe_fd[1]);
    char buf[BUFSIZE];
    ssize_t bytes;
    if ((bytes = read(pipe_fd[0], buf, sizeof(buf))) > 0) {
        perror(buf);
        errNum = 1;
    } else {
        errNum = 0;
    }
    int wstatus;
    int tpid = wait(&wstatus);   // wait for child to finish
    free(lineBufferCopy);
    return;
}

// process line
void dumpLine(void)
{
    char *token;
    char *delim = " \t\n";
    int l = 0, r = linePos - 2;
    char c, cwd [BUFSIZE];
    assert(lineBuffer[linePos-1] == '\n');
    char *lineBufferCopy = malloc(BUFSIZE);
    strcpy(lineBufferCopy,lineBuffer);
    token = strtok(lineBufferCopy, delim);
    const char *commands[3] = {"cd", "pwd", "exit"};
    char *defaultDirs[6] = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin"};
    DIR *dp;
    struct dirent *de;
    struct stat sfile;
    int commandSize = 3, defaultDirSize = 6, input_fd, output_fd;
   
    if (token != NULL) {
        //exit case
        if (strcmp(token, "exit") == 0) {
            fputs("Terminating Program\n", stderr);
            exitShell = 1; 
            return;

        //pwd case
        } else if (strcmp(token, "pwd") == 0) {
            if (token = strtok(NULL, delim)){
                fputs("Unexpected Number of Arguments\n",stderr);
                errNum = 1;
            }
            else{
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    fprintf(stderr, "Current working directory: %s\n", cwd);
                    errNum = 0;
                } else {
                    fputs("getcwd() error\n", stderr);
                    errNum = 1;
                }
            }
            return;
        
        //cd case
        } else if (strcmp(token, "cd") == 0) {
            token = strtok(NULL, delim);
            if(!token){
                //change directory to home
                if (chdir(getenv("HOME")) != 0) {
                    errNum = 1;
                    perror("Failed");
                } else {
                    fprintf(stderr, "Success\n");
                    fprintf(stderr, "New Dir: %s\n", getcwd(cwd, sizeof(cwd)));
                    errNum = 0;
                }
            }
            else{
                //change directory
                char *dir = getcwd(cwd, sizeof(cwd));
                strcat(dir, "/");
                strcat(dir, token);
                char *nextToken = strtok(NULL, delim);
                if (DEBUG) printf("token %s\n", nextToken);
                if (strcmp(token, "..") == 0) {
                    if(nextToken == NULL){
                        if (chdir("..") != 0) {
                            errNum = 1;
                            perror("Failed");
                        } else {
                            fprintf(stderr, "Success\n");
                            fprintf(stderr, "New Dir: %s\n", getcwd(cwd, sizeof(cwd)));
                            errNum = 0;
                        }
                    }
                    else{
                        fputs("Unexpected Number of Arguments\n",stderr);
                        errNum = 1;
                    }
                    
                } else if (nextToken != NULL || chdir(dir) != 0) {
                    fputs("Unexpected Number of Arguments\n",stderr);
                    errNum = 1;
                } else {
                    fprintf(stderr, "Success\n");
                    fprintf(stderr, "New Dir: %s\n", getcwd(cwd, sizeof(cwd)));
                    errNum = 0;
                }
                return;
            }
        
        //path file case
        } else if (token[0] == '/'){
            char dir[BUFSIZE];
            strcpy(dir, token);
            executeLine(dir,token);
            return;

        //home dir path file case
        } else if (token[0] == '~' && token[1] == '/') {
            char *homeDir = getenv("HOME");
            fprintf(stderr,"%s\n",homeDir);

            //replace ~ with homedir
        
        //invalid argument case
        } else if (strcmp(token, ">") == 0 || strcmp(token, "<") == 0){
            fputs("Invalid Arguments",stderr);
            errNum = 1;
            return;
        
        //defaultDirs command or nonexistent command case
        } else {
            //search defaultDirs for command
            int found = 0; //command found?
            for (int j = 0; j < defaultDirSize; j++) {
                dp = opendir(defaultDirs[j]);
                if (!dp) {
                    errNum = 1;
                    fprintf(stderr, "Error Opening Directory %s\n", defaultDirs[j]);
                } else {
                    char dir[BUFSIZE];
                    sprintf(dir, "%s/%s", defaultDirs[j], token);
                    if (stat(dir, &sfile) == 0) {
                        found = 1;
                        executeLine(dir, token);
                        break;

                    } else {
                        errNum = 1;
                    }
                }
            }
            if(!found){
                fputs("Error: Command Not Found\n",stderr);
            }
        }
    }
    free(lineBufferCopy);
}