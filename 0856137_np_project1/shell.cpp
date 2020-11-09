#include "shell.h"
#include <bits/stdc++.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <typeinfo>
#include <unistd.h>

#define CommandLen 15001
#define NUMPIPE_SIZE 56789
#define STDOUT 1
using namespace std;

int main()
{
    int CommamdCount = 0;
    bool Needtowait = false;
    pid_t lastpid;
    string Input;
    NumberPipe Number_pipe[NUMPIPE_SIZE];

    queue<command> Command_que;
    queue<string> Input_cmd, Path_que;
    setenv("PATH", "bin:.", 1);
    while (cout << "% " && getline(cin, Input)) {
        // cout << Input << " is input" << endl;
        ClearQueue(Input_cmd);
        Input_cmd = SplitCommand(Input);
        while (!Input_cmd.empty()) {
            CommamdCount++;
            CommamdCount %= NUMPIPE_SIZE;
            command cmd;

            cmd.cmd = Input_cmd.front();
            cmd.options = ResolveCommands(Path_que, cmd.cmd);
            // cout << cmd.cmd << endl;

            Input_cmd.pop();
            while (!Input_cmd.empty() && Input_cmd.front()[0] != '|' && Input_cmd.front()[0] != '!') {
                // extract parameter
                cmd.parameter.push_back(Input_cmd.front());
                Input_cmd.pop();
            }
            if (!Input_cmd.empty() && isPipeNum(Input_cmd.front())) {
                // Check is pipe or Numbered-Pipes
                SetPipeNum(Input_cmd.front(), cmd, CommamdCount);
                Input_cmd.pop();
            } else {
                cmd.pip_num = -1;
            }
            cmd.cmd_cnt = CommamdCount;
            Command_que.push(cmd);
        }
        while (!Command_que.empty()) {
            command cmd = Command_que.front();
            string env;
            // cout << "cmd " << cmd.cmd << " " << cmd.pip_num << endl;
            switch (cmd.options) {
            case SetEnv:
                if (cmd.parameter.size() != 2) {
                    fprintf(stderr, "Error: Invalid parameter of setenv.\n");
                } else {
                    setenv(cmd.parameter[0].c_str(), cmd.parameter[1].c_str(), 1);
                }
                break;
            case PrintEnv:
                env = getenv(cmd.parameter[0].c_str());
                cout << env << endl;
                break;
            case EXIT:
                return 0;
            case EXEC:
                ExecuteCommand(Path_que, cmd, Number_pipe,
                    lastpid);
                break;
            default:
                cout << "No options!" << endl;
            }
            Needtowait = (cmd.pip_num == -1);
            Command_que.pop();
        }
        int status;

        if (Needtowait)
            waitpid(lastpid, &status, 0);
    }

    return 0;
}

queue<string> SplitCommand(string input) // Seperate input command with space
{
    queue<string> cmd_split;
    istringstream istr(input);
    string str;

    while (istr >> str) {
        cmd_split.push(str);
    }
    return cmd_split;
}

CommandOptions ResolveCommands(queue<string>& path, string input)
{
    if (input == "setenv")
        return SetEnv;
    if (input == "printenv")
        return PrintEnv;
    if (input == "exit")
        return EXIT;
    return EXEC;
}

inline int isPipeNum(string input)
{
    if (input[1] != ' ')
        return true;
    return 0;
}

inline void SetPipeNum(string input, command& cmd, int CmdCnt)
{
    if (input[0] == '!')
        cmd.isExclamationmark = true;
    if (input == "|" || input == "!")
        cmd.pip_num = (CmdCnt + 1) % NUMPIPE_SIZE;
    else {
        input.erase(input.begin());
        cmd.pip_num = (stoi(input) + CmdCnt) % NUMPIPE_SIZE;
    }
    return;
}

void SetPATH(queue<string>& Path_que, string path)
{
    // cout << path;
    ClearQueue(Path_que);
    size_t pos;

    while (!path.empty()) {
        pos = path.find_first_of(":", 0);
        if (pos != string::npos) {
            Path_que.push(path.substr(0, pos));
            path.erase(path.begin(), path.begin() + pos + 1);
        } else {
            Path_que.push(path);
            path.erase(path.begin(), path.end());
        }
        // cout << Path_que.back() << endl;
    }
    return;
}

void ExecuteCommand(queue<string>& path, command cmd,
    NumberPipe* Number_pipe, pid_t& pid)
{
    int count = cmd.cmd_cnt, pipenum = cmd.pip_num;

    if (pipenum != -1 && !Number_pipe[pipenum].ispiped) {
        if (pipe(Number_pipe[pipenum].pipe) < 0) {
            fprintf(stderr, "Error: Unable to create pipe. (%d)\n", pipenum);
            exit(EXIT_FAILURE);
        }
        Number_pipe[pipenum].ispiped = true;
    }
    // cout << "pid: " << getpid() << ", cur_line: " << count << ", pipe to: " << pipenum << ", isPiped: " << Number_pipe[count].ispiped << endl;
    signal(SIGCHLD, ChildHandler);
    while ((pid = fork()) < 0)
        usleep(1000);

    if (pid == 0) {
        // child
        /* INPUT */
        if (Number_pipe[count].ispiped) {
            // Other process pipe to this process
            close(Number_pipe[count].pipe[1]);
            dup2(Number_pipe[count].pipe[0], STDIN_FILENO);
            close(Number_pipe[count].pipe[0]);
        }
        /* OUTPUT */
        if (cmd.isExclamationmark) {
            dup2(Number_pipe[pipenum].pipe[1], STDERR_FILENO);
        }
        if (pipenum != -1 && Number_pipe[pipenum].ispiped) {
            // pipe to other process
            close(Number_pipe[pipenum].pipe[0]);
            dup2(Number_pipe[pipenum].pipe[1], STDOUT_FILENO);
            close(Number_pipe[pipenum].pipe[1]);
        }

        if (cmd.parameter.size() >= 2 && cmd.parameter[cmd.parameter.size() - 2] == ">") {
            // cout << "enter file write" << endl;
            int FileDescriptor = open(cmd.parameter[cmd.parameter.size() - 1].c_str(),
                O_RDWR | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

            dup2(FileDescriptor, STDOUT_FILENO);
            close(FileDescriptor);
            cmd.parameter.erase(cmd.parameter.end());
            cmd.parameter.erase(cmd.parameter.end());
        }

        char* arg[cmd.parameter.size() + 2];

        arg[0] = strdup(cmd.cmd.c_str());
        arg[cmd.parameter.size() + 1] = NULL;
        for (unsigned int i = 1; i <= cmd.parameter.size(); i++)
            arg[i] = strdup(cmd.parameter[i - 1].c_str());
        // Number_pipe[count].ispiped = false;
        for (unsigned int i = 0; i < sizeof(Number_pipe) / sizeof(NumberPipe);
             i++) {
            close(Number_pipe[i].pipe[0]);
            close(Number_pipe[i].pipe[1]);
        }
        if (execvp(arg[0], arg) < 0)
            cerr << "Unknown command: [" + cmd.cmd + "]." << endl;

        exit(0);
    } else {
        // parent
        if (Number_pipe[count].ispiped) {
            close(Number_pipe[count].pipe[1]);
            close(Number_pipe[count].pipe[0]);
            Number_pipe[count].ispiped = false;
        }
    }
    return;
}

void ChildHandler(int signo)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // do nothing
    }
    return;
}

inline void ClearQueue(queue<string>& que)
{
    while (!que.empty())
        que.pop();
    return;
}
