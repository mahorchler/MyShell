CC = gcc
all: mysh
mysh: mysh.c redirection.c wildcards.c
	$(CC) -o mysh mysh.c 
clean:
	rm -f mysh *.o