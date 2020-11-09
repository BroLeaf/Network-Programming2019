#include "shell.h"
#include <bits/stdc++.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <typeinfo>
#include <unistd.h>

#define MAX_CLIENT_NUM 56789
#define STDOUT 1
#define QUEUE_NUM 1
using namespace std;

int main(int argc, char* argv[])
{
    clearenv();
    setenv("PATH", "bin:.", 1);
    // signal(SIGCHLD, ChildHandler);
    signal(SIGCHLD, ChildHandler);
    int server_socketfd = 0, client_socketfd = 0;
    server_socketfd = socket(AF_INET, SOCK_STREAM, 0);

    // config
    struct sockaddr_in serverinfo, clientinfo;
    socklen_t addrlen = sizeof(clientinfo);
    bzero(&serverinfo, sizeof(serverinfo));
    serverinfo.sin_family = PF_INET;
    serverinfo.sin_addr.s_addr = INADDR_ANY;
    serverinfo.sin_port = htons(atoi(argv[1]));

    int flag = 1;
    if (setsockopt(server_socketfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        std::cerr << "Flag Error" << std::endl;
    }
    if (setsockopt(server_socketfd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(int)) < 0) {
        std::cerr << "Flag Error" << std::endl;
    }

    if (bind(server_socketfd, (struct sockaddr*)&serverinfo, sizeof(serverinfo)) < 0)
        cerr << "!! Bind ERROR !!" << endl;

    if (listen(server_socketfd, MAX_CLIENT_NUM) < 0)
        cerr << "!! Lensten ERROR !!" << endl;

    while (1) {
        client_socketfd = accept(server_socketfd, (struct sockaddr*)&clientinfo, &addrlen);
        int childpid;
        if ((childpid = fork()) < 0)
            cerr << "!! Fork ERROR !!" << endl;
        if (childpid == 0) {
            // Child
            close(server_socketfd);
            dup2(client_socketfd, STDOUT_FILENO);
            dup2(client_socketfd, STDERR_FILENO);
            // char* shell[1];
            // string str = "npshell";
            // shell[0] = strdup("npshell");
            // if(execvp(shell[0], shell) < 0)
            //     cerr << "!! Execute ERROR !!" << endl;
            Shell(client_socketfd);
            close(client_socketfd);

            exit(0);
        } else {
            // parent
            close(client_socketfd);
        }
    }
    return 0;
}
