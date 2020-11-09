CC=g++
CFLAGS=-std=c++14 -pedantic -pthread -lboost_system -lboost_filesystem
TUNE= -O2
SOURCE_PROXY=socks_server.cpp
SOURCE_SERVER=http_server.cpp connection_manager.cpp request_parser.cpp
SOURCE_CONSOLE=console.cpp session.cpp
PROXY=socks_server
SERVER=http_server
CONSOLE=hw4.cgi

all: target

run: target
	./$(SERVER)

target: $(SOURCE)
	$(CC) -o $(PROXY) $(SOURCE_PROXY) $(CFLAGS)
	$(CC) -o $(CONSOLE) $(SOURCE_CONSOLE) $(CFLAGS)

proxy: $(SOURCE_PROXY)
	$(CC) -o $(PROXY) $(SOURCE_PROXY) $(CFLAGS)

p3: $(SOURCE_CONSOLE)
	$(CC) -o $(CONSOLE) $(SOURCE_CONSOLE) $(CFLAGS) -Wall

.PHONY:clean

clean:
	$(RM) $(PROXY) $(CONSOLE)
