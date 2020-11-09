CC=g++
CFLAGS= -Wall
TUNE= -O
SOURCE= shell.cpp
EXEC= npshell

all: target

run: target
	./npshell

test: debug
	gdb -q npshell

target: $(SOURCE)
	$(CC) $(CFLAGS) -o npshell shell.cpp

debug: $(SOURCE)
	$(CC) $(CFLAGS) -g -o npshell shell.cpp

.PHONY:clean

clean:
	$(RM) $(EXEC)
