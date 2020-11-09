#include "shell.h"

using namespace std;

int main(int argc, char* argv[])
{
    signal(SIGCHLD, ChildHandler);
    fd_set master, read_fds;
    int fdmax, fd_stdout, fd_stderr;
    Client* clientset = (Client*)malloc(MAX_CLIENT_NUM * sizeof(Client));
    NumberPipe** number_pipe;
    UserPipe** user_pipe;
    map<string, string> env[MAX_CLIENT_NUM];
    int serversocketfd = 0, clientSocketfd = 0;
    serversocketfd = socket(AF_INET, SOCK_STREAM, 0);

    number_pipe = (NumberPipe**)malloc(MAX_CLIENT_NUM * sizeof(NumberPipe*));
    for (int i = 0; i < NUMPIPE_SIZE; i++)
        number_pipe[i] = (NumberPipe*)malloc(NUMPIPE_SIZE * sizeof(number_pipe));
    user_pipe = (UserPipe**)malloc((MAX_CLIENT_NUM) * sizeof(UserPipe*));
    for (int i = 0; i < MAX_CLIENT_NUM; i++)
        user_pipe[i] = (UserPipe*)malloc(MAX_CLIENT_NUM * sizeof(UserPipe));
    for (int i = 0; i < MAX_CLIENT_NUM; i++) {
        for (int j = 0; j < NUMPIPE_SIZE; j++) {
            number_pipe[i][j].pipe[0] = -1;
            number_pipe[i][j].pipe[1] = -1;
            number_pipe[i][j].ispiped = false;
        }
        for (int k = 0; k < MAX_CLIENT_NUM; k++) {
            user_pipe[i][k].pipe[0] = -1;
            user_pipe[i][k].pipe[1] = -1;
            user_pipe[i][k].ispiped = false;
        }
    }
    // config
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    struct sockaddr_in serverinfo, clientinfo;
    socklen_t addrlen = sizeof(clientinfo);
    bzero(&serverinfo, sizeof(serverinfo));
    serverinfo.sin_family = PF_INET;
    serverinfo.sin_addr.s_addr = INADDR_ANY;
    serverinfo.sin_port = htons(atoi(argv[1]));

    int flag = 1;
    if (setsockopt(serversocketfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        std::cerr << "Flag Error" << std::endl;
    }
    if (setsockopt(serversocketfd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(int)) < 0) {
        std::cerr << "Flag Error" << std::endl;
    }

    if (bind(serversocketfd, (struct sockaddr*)&serverinfo, sizeof(serverinfo)) < 0)
        cerr << "!! Bind ERROR !!" << endl;

    if (listen(serversocketfd, MAX_CLIENT_NUM) < 0)
        cerr << "!! Lensten ERROR !!" << endl;

    /* Add listener to master */
    FD_SET(serversocketfd, &master);
    fdmax = serversocketfd;

    priority_queue<int, vector<int>, greater<int>> id_queue;
    memset(clientset, 0, sizeof(Client) * MAX_CLIENT_NUM);
    for (int i = 1; i < MAX_CLIENT_NUM; i++)
        id_queue.push(i);
    while (1) {
        clearenv();
        char buffer[MAX_MASSAGE_SIZE];
        memset(buffer, 0, MAX_MASSAGE_SIZE);
        int nbytes;

        read_fds = master;

        while ((select(fdmax + 1, &read_fds, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR) {
                usleep(500);
            } else {
                cerr << strerror(errno) << endl;
                cerr << "!! select ERROR." << endl;
                usleep(500);
                exit(1);
            }
        }
        /* select return > 0 */
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // find new connection
                if (i == serversocketfd) {
                    clientSocketfd = accept(serversocketfd, (struct sockaddr*)&clientinfo, &addrlen);
                    if (clientSocketfd == -1) {
                        cerr << "!!accept error" << endl;
                        continue;
                    } else {
                        FD_SET(clientSocketfd, &master); // Add to master
                        if (clientSocketfd > fdmax)
                            fdmax = clientSocketfd;
                    }
                    Welcome(clientSocketfd);

                    /* Initialize client config */
                    int clientid = id_queue.top();
                    id_queue.pop();
                    cout << "new client id: " << clientid << endl
                         << flush;
                    char addr[INET_ADDRSTRLEN];
                    inet_ntop(clientinfo.sin_family, &(clientinfo.sin_addr), addr, INET_ADDRSTRLEN);
                    strncpy(clientset[clientid].addr, addr, INET_ADDRSTRLEN);
                    strncpy(clientset[clientid].name, "(no name)", 20);
                    clientset[clientid].port = ntohs(clientinfo.sin_port);
                    clientset[clientid].sockfd = clientSocketfd;
                    clientset[clientid].used = true;
                    clientset[clientid].cmd_cnt = 0;
                    env[clientid].clear();
                    env[clientid]["PATH"] = "bin:.";
                    string loginmsg;
                    loginmsg = "*** User \'" + (string)clientset[clientid].name + "\' entered from " + clientset[clientid].addr
                        + ":" + to_string(clientset[clientid].port) + ". ***\n";
                    Broadcast(loginmsg.c_str(), clientset);
                    if (send(clientset[clientid].sockfd, "% ", 2, 0) == -1) {
                        cerr << "!! send \'%\' ERROR" << endl
                             << flush;
                        exit(1);
                    }
                } else {
                    int requset_fd = i;
                    int index;
                    for (index = 1; index < MAX_MASSAGE_SIZE; index++) {
                        /* Get client id */
                        if (clientset[index].sockfd == requset_fd) {
                            break;
                        }
                    }

                    if ((nbytes = recv(i, buffer, MAX_MASSAGE_SIZE - 1, 0) > 0)) {
                        /* handle client request */

                        for (unsigned int j = 0; j < strlen(buffer); j++) {
                            if (buffer[j] == '\r' || buffer[j] == '\n' || buffer[j] == '\0')
                                memset(&buffer[j], 0, 1);
                        }
                        // cout << "byte: " << strlen (buffer) << endl << buffer << endl << flush;

                        fd_stderr = dup(STDERR_FILENO);
                        fd_stdout = dup(STDOUT_FILENO);
                        dup2(requset_fd, STDOUT_FILENO);
                        dup2(requset_fd, STDERR_FILENO);
                        int user_status = Shell(buffer, &clientset, index, number_pipe, user_pipe, env);
                        dup2(fd_stdout, STDOUT_FILENO);
                        dup2(fd_stderr, STDERR_FILENO);
                        close(fd_stderr);
                        close(fd_stdout);
                        if (user_status < 0) {
                            // user exit
                            cout << "id: " << index << " leave." << endl;
                            id_queue.push(index);
                            // memset(&clientset[index], 0, sizeof(Client));
                            // Logout(clientset, index);
                            closepipe(number_pipe, user_pipe, index);
                            close(requset_fd); // bye!
                            FD_CLR(requset_fd, &master); // 從 master set 中移除
                            // continue;
                        } else {
                            if (write(requset_fd, "% ", 2) == -1) {
                                cerr << "!! send \'%\' ERROR" << endl
                                     << flush;
                                exit(1);
                            }
                        }
                        memset(buffer, 0, MAX_MASSAGE_SIZE);
                    } else {
                        /* client leave with unknown reason*/
                        closepipe(number_pipe, user_pipe, index);
                        cout << "id: " << index << " leave." << endl;
                        id_queue.push(index);
                        Logout(clientset, index);
                        memset(&clientset[index], 0, sizeof(Client));
                        close(requset_fd); // bye!
                        FD_CLR(requset_fd, &master); // 從 master set 中移除
                        cout << flush;
                    }
                }
            }
        }
    }
    return 0;
}
