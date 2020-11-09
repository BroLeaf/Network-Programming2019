#include <bits/stdc++.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <typeinfo>
#include <unistd.h>

using namespace std;

enum CommandOptions {
    SetEnv,
    PrintEnv,
    EXIT,
    EXEC
};

struct Command {
    string cmd;
    vector<string> parameter;
    CommandOptions options;
    int pipe_num;
    int cmd_cnt;
    bool isExclamationmark;
    bool isRedirect;
};

struct NumberPipe {
    int pipe[2];
    bool ispiped;
};

void Shell(int clientSocketfd);
queue<string> SplitCommand(string input);
CommandOptions ResolveCommands(queue<string>& path, string input);
inline int IsPipeNum(string input);
inline void SetPipeNum(string input, Command& cmd, int CmdCnt);
inline void ErasePrefixSpace(string& input);
void SeperateParameter(vector<string>& parameter, string par);
void SetPATH(queue<string>& Path_que, string path);
void ExecuteCommand(queue<string>& path, Command cmd, NumberPipe* Number_pipe, pid_t& pid);
inline void ClearQueue(queue<string>& que);
void ChildHandler(int signo);
