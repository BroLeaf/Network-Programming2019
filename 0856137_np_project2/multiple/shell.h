#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <typeinfo>
#include <unistd.h>

#define COMMANDLEN 15001
#define NUMPIPE_SIZE 56789
#define STDOUT 1
#define SHAREMEM_KEY ((key_t)7680)
#define SEMAPHORE_KEY ((key_t)7253)
#define FLAG 0666
#define MAX_CLIENT_NUM 31
#define MAX_MASSAGE_SIZE 1024
using namespace std;

enum CommandOptions {
    SetEnv,
    PrintEnv,
    EXIT,
    EXEC,
    who,
    name,
    tell,
    yell
};

struct Command {
    string cmd;
    string total_line;
    vector<string> parameter;
    CommandOptions options;
    int pipe_num;
    int cmd_cnt;
    int user_pipe_to;
    int user_pipe_from;
    bool isExclamationmark;
    bool isRedirect;
};

struct NumberPipe {
    int pipe[2];
    bool ispiped;
};

struct UserPipe {
    int pipe[2];
    bool ispiped;
};

struct Client {
    bool used;
    pid_t pid;
    int id;
    int port;
    char addr[INET6_ADDRSTRLEN];
    char msg[1025];
    char name[20];
    int user_pipe_from[31]; // 1 means have pipe processed yet, 0 means have opened the file, -1 means nothing.
    int fd_table[31];
};

union semun {
    int val; /* value for SETVAL */
    struct semid_ds* buf; /* buffer for IPC_STAT, IPC_SET */
    unsigned short* array; /* array for GETALL, SETALL */
    struct seminfo* __buf; /* buffer for IPC_INFO */
};
void Shell(sockaddr_in ClientSocketfd);
queue<string> SplitCommand(string input);
CommandOptions ResolveCommands(queue<string>& path, string input);
void ParseCommand(Command& cmd, queue<string>& input_cmd);
inline void SetCommandIO(Command& cmd, queue<string>& input_cmd);
inline bool IsRedirect(string str);
inline bool IsPipe(string str);
inline bool IsExecutionCmd(CommandOptions type);
void ExecuteCommand(Command cmd, NumberPipe* Number_pipe, pid_t& pid);
inline void ClearQueue(queue<string>& que);
void ChildHandler(int signo);
void Broadcast(const char* msg);
void InitialClient(sockaddr_in clientSocketfd);
void Welcome();
void Login();
void Logout();
void Who();
void Name(Command cmd);
void Tell(Command cmd);
void Yell(Command cmd);
