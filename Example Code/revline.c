#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef BUFSIZE
#define BUFSIZE 512
#endif


char *lineBuffer;
int linePos, lineSize;

void append(char *, int);
void dumpLine(void);


int main(int argc, char **argv)
{
    int fin, bytes, pos, lstart;
    char buffer[BUFSIZE];

    // open specified file or read from stdin
    if (argc > 1) {
	fin = open(argv[1], O_RDONLY);
	if (fin == -1) {
	    perror(argv[1]);
	    exit(EXIT_FAILURE);
	}
    } else {
	fin = 0;
    }
    // remind user if they are running in interactive mode
    if (isatty(fin)) {
	fputs("[Reading from terminal]\n", stderr);
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
    }
    if (linePos > 0) {
	// file ended with partial line
	append("\n", 1);
	dumpLine();
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

// print the contents of crntLine in reverse order
// requires: 
// - linePos is the length of the line in lineBuffer
// - linePos is at least 1
// - final character of current line is '\n'
void dumpLine(void)
{
    int l = 0, r = linePos - 2;
    char c;
    assert(lineBuffer[linePos-1] == '\n');

    // reverse contents of crntLine
    while (l < r) {
	c = lineBuffer[l];
	lineBuffer[l] = lineBuffer[r];
	lineBuffer[r] = c;
	++l;
	--r;
    }

    // dump output to stdout
    write(1, lineBuffer, linePos);
    // FIXME should confirm that all bytes were written
}
