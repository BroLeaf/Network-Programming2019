CC=g++
CFLAGS=-std=c++14 -pedantic -pthread -lboost_system -lboost_filesystem
TUNE= -O2
SOURCE_SERVER= http_server.cpp connection_manager.cpp request_parser.cpp
SOURCE_CONSOLE=console.cpp session.cpp
SERVER= http_server
CONSOLE=console.cgi
EXEC= $(s1) $(s2) $(s3)

all: target

run: target
	./$(SERVER)

target: $(SOURCE)
	$(CC) -o $(SERVER) $(SOURCE_SERVER) $(CFLAGS)
	$(CC) -o $(CONSOLE) $(SOURCE_CONSOLE) $(CFLAGS) -Wall

.PHONY:clean

clean:
	$(RM) $(SERVER) $(CONSOLE)
