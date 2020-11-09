CC=g++
CFLAGS= -Wall
TUNE= -O
SOURCE= simple/simple.cpp single/single.cpp multiple/multiple.cpp
s1= np_simple
s2= np_single_proc
s3= np_multi_proc
EXEC= $(s1) $(s2) $(s3)

all: target

run: target
	./np_simple

test: debug
	gdb -q np_single_proc

s1:
	$(CC) $(CFLAGS) -o np_simple simple/simple.cpp simple/shell.cpp
	./$(s1) 65123

s2:
	$(CC) $(CFLAGS) -o np_single_proc single/single.cpp single/shell.cpp 
	./$(s2) 65123

s3:
	$(CC) $(CFLAGS) -o np_multi_proc multiple/multiple.cpp multiple/shell.cpp 
	./$(s3) 65123

target: $(SOURCE)
	$(CC) $(CFLAGS) -o np_simple simple/simple.cpp simple/shell.cpp
	$(CC) $(CFLAGS) -o np_single_proc single/single.cpp single/shell.cpp 
	$(CC) $(CFLAGS) -o np_multi_proc multiple/multiple.cpp multiple/shell.cpp 

debug: $(SOURCE)
	$(CC) $(CFLAGS) -g -o np_single_proc single/single.cpp single/shell.cpp

.PHONY:clean

clean:
	$(RM) $(EXEC)
