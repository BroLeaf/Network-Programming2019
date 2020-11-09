#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <typeinfo>
#include <unistd.h>

#define COMMANDLEN 15001
#define NUMPIPE_SIZE 12345
#define STDOUT 1
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
    int cmd_cnt;
    int pipe_num;
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
    int cmd_cnt;
    int port;
    int sockfd;
    char addr[INET6_ADDRSTRLEN];
    char msg[1025];
    char name[20];
    // int user_pipe_from[31];     // 1 means have pipe processed yet, 0 means have opened the file, -1 means nothing.
    // map<string, string> env;
};

int Shell(string input, Client** clientset, int userid, NumberPipe** numberpipe, UserPipe** user_pipe, map<string, string>* env);
queue<string> SplitCommand(string input);
CommandOptions ResolveCommands(queue<string>& path, string input);
void ParseCommand(Command& cmd, queue<string>& input_cmd);
inline void SetCommandIO(Command& cmd, queue<string>& input_cmd);
inline bool IsExecutionCmd(CommandOptions cmdtype);
inline bool IsRedirect(string str);
inline bool IsPipe(string str);
inline void SetUserPipe(Command& cmd);
inline int IsPipeNum(string input);
inline void SetPipeNum(string input, Command& cmd, int CmdCnt);
inline void ErasePrefixSpace(string& input);
inline void ErasePrefixSpace(string& input);
void SeperateParameter(vector<string>& parameter, string par);
void ExecuteCommand(Command cmd, Client** clientset, int userid, pid_t& pid, NumberPipe** numberpipe, UserPipe** user_pipe);
inline void ClearQueue(queue<string>& que);
void ChildHandler(int signo);
void Broadcast(const char* msg, Client* clientset);
void Welcome(int clientSocketfd);
void Logout(Client* clientset, int userid);
void Who(Client* clientset, int userid);
void Name(Command cmd, Client* clientset, int userid);
void Tell(Command cmd, Client* clientset, int userid);
void Yell(Command cmd, Client* clientset, int userid);
void closepipe(NumberPipe** numberpipe, UserPipe** userpipe, int userid);
void Resetenv(map<string, string>* env, int userid);
