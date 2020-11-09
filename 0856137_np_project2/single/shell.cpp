#include "shell.h"

using namespace std;

// NumberPipe client.number_pipe[NUMPIPE_SIZE];

int Shell(string input, Client** clientset, int userid, NumberPipe** numberpipe, UserPipe** user_pipe, map<string, string>* env)
{
    // cout << input << endl;
    bool Needtowait = false;
    pid_t lastpid;

    queue<Command> command_que;
    queue<string> input_cmd, Path_que;
    Resetenv(env, userid);
    // setenv("PATH", "bin:.", 1);

    input_cmd = SplitCommand(input);
    while (!input_cmd.empty()) {

        (*clientset)[userid].cmd_cnt++;
        (*clientset)[userid].cmd_cnt %= NUMPIPE_SIZE;
        Command cmd;
        // memset(&cmd, 0, sizeof(Command));
        cmd.total_line = input;
        cmd.parameter.clear();
        // pop '\r' or '\n'
        if (cmd.total_line.back() == '\r' || cmd.total_line.back() == '\n')
            cmd.total_line.pop_back();
        cmd.cmd = input_cmd.front();
        input_cmd.pop();
        cmd.options = ResolveCommands(Path_que, cmd.cmd);
        cmd.cmd_cnt = (*clientset)[userid].cmd_cnt;
        cmd.pipe_num = -1;
        cmd.user_pipe_from = -1;
        cmd.user_pipe_to = -1;
        cmd.isExclamationmark = false;
        cmd.isRedirect = false;

        ParseCommand(cmd, input_cmd);
        command_que.push(cmd);
    }
    while (!command_que.empty()) {
        Command cmd = command_que.front();
        // cout << "cmd " << cmd.cmd << " " << cmd.pipe_num << endl;
        switch (cmd.options) {
        case SetEnv:
            if (cmd.parameter.size() != 2) {
                fprintf(stderr, "Error: Invalid parameter of setenv.\n");
            } else {
                // map <string, string>::iterator it;
                // if ((it = env[userid].find(cmd.parameter[0]) != env[userid].end())) {
                // 	it->second = cmd.parameter[1];
                // } else {
                // 	env[userid].insert(it, pair<string, string>(cmd.parameter[0], cmd.parameter[1]));
                // }
                env[userid][cmd.parameter[0]] = cmd.parameter[1];
                // env.emplace(cmd.parameter[0], cmd.parameter[1]);
                // cout << env[userid][cmd.parameter[0]] << endl << cmd.parameter[1];
                setenv(cmd.parameter[0].c_str(), cmd.parameter[1].c_str(), 1);
            }
            break;
        case PrintEnv:
            if (getenv(cmd.parameter[0].c_str()))
                cout << getenv(cmd.parameter[0].c_str()) << endl;
            break;
        case who:
            Who((*clientset), userid);
            break;
        case name:
            Name(cmd, (*clientset), userid);
            break;
        case tell:
            Tell(cmd, (*clientset), userid);
            break;
        case yell:
            Yell(cmd, (*clientset), userid);
            break;
        case EXIT:
            Logout((*clientset), userid);
            return -1;
            break;
        case EXEC:
            ExecuteCommand(cmd, clientset, userid, lastpid, numberpipe, user_pipe);
            break;
        default:
            cout << "No options!" << endl;
        }
        Needtowait = (cmd.pipe_num == -1);
        command_que.pop();
    }
    int status;
    if (Needtowait)
        waitpid(lastpid, &status, 0);
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
    if (input == "who")
        return who;
    if (input == "name")
        return name;
    if (input == "tell")
        return tell;
    if (input == "yell")
        return yell;
    if (input == "exit")
        return EXIT;
    return EXEC;
}

void ParseCommand(Command& cmd, queue<string>& input_cmd)
{
    /* pipe include user pipe and number pipe, redirect means redirect output to file */
    while (!input_cmd.empty()) {
        /* Regard I/O as the end of command */
        if (IsExecutionCmd(cmd.options) && (IsPipe(input_cmd.front()) || IsRedirect(input_cmd.front()))) {
            SetCommandIO(cmd, input_cmd);
            break;
        }
        cmd.parameter.push_back(input_cmd.front());
        input_cmd.pop();
    }
#if 0 /* old version of parser */
    while (!input_cmd.empty() && input_cmd.front()[0] != '|' && input_cmd.front()[0] != '!') {
        if (input_cmd.front().size() > 1 && (input_cmd.front()[0] == '<' || input_cmd.front()[0] == '>')) {
            // cout << "cmd: " << input_cmd.front() << " string size: " << input_cmd.front().size() << endl << flush;
            // Handle user pipe
            cmd.parameter.push_back(input_cmd.front());
            input_cmd.pop();
            if (!input_cmd.empty() && input_cmd.front().size() > 1 && (input_cmd.front()[0] == '<' || input_cmd.front()[0] == '>')) {
                // cout << "cmd: " << input_cmd.front() <<  "string size: " << input_cmd.front().size() << endl << flush;
                cmd.parameter.push_back(input_cmd.front());
                input_cmd.pop();
            } else if (input_cmd.size() > 1 && input_cmd.front() == ">" && cmd.parameter.back()[0] == '<') {
                // handle fucking case: cmd <(#user) > (filename)
                cmd.parameter.push_back(input_cmd.front());
                input_cmd.pop();
                cmd.parameter.push_back(input_cmd.front());
                input_cmd.pop();
            }
            break;
        }
        // extract parameter from input line to cmd struct
        cmd.parameter.push_back(input_cmd.front());
        input_cmd.pop();
    }
#endif
}

inline void SetCommandIO(Command& cmd, queue<string>& input_cmd)
{
    /* because in/out may reverse, so need to check twice */
    for (int i = 0; i < 2 && !input_cmd.empty(); i++) {
        switch (input_cmd.front()[0]) {
        case '>':
            if (IsRedirect(input_cmd.front())) {
                cmd.isRedirect = true;
                cmd.parameter.push_back(input_cmd.front()); // push ">"
                input_cmd.pop();
                cmd.parameter.push_back(input_cmd.front()); // push filename
                input_cmd.pop();
            } else {
                input_cmd.front().erase(input_cmd.front().begin());
                // cout << "Pipe to " << input_cmd.front() << endl << flush;
                cmd.user_pipe_to = stoi(input_cmd.front());
                input_cmd.pop();
            }
            break;
        case '<':
            input_cmd.front().erase(input_cmd.front().begin());
            cmd.user_pipe_from = stoi(input_cmd.front());
            input_cmd.pop();
            break;
        case '!':
            cmd.isExclamationmark = true;
        case '|':
            if (input_cmd.front() == "|" || input_cmd.front() == "!")
                cmd.pipe_num = (cmd.cmd_cnt + 1) % NUMPIPE_SIZE;
            else {
                input_cmd.front().erase(input_cmd.front().begin());
                cmd.pipe_num = (stoi(input_cmd.front()) + cmd.cmd_cnt) % NUMPIPE_SIZE;
            }
            input_cmd.pop();
            break;
        }
    }
}

inline bool IsExecutionCmd(CommandOptions cmdtype)
{
    return (cmdtype == EXEC);
}

inline bool IsRedirect(string str)
{
    return (str == ">");
}

inline bool IsPipe(string str)
{
    bool userpipe = false, numberpipe = false;
    if (str.size() > 1 && (str[0] == '>' || str[0] == '<'))
        userpipe = true;
    if (str[0] == '|' || str[0] == '!')
        numberpipe = true;
    return (userpipe | numberpipe);
}

void ExecuteCommand(Command cmd, Client** clientset, int userid, pid_t& pid, NumberPipe** numberpipe, UserPipe** user_pipe)
{
    int count = cmd.cmd_cnt, pipenum = cmd.pipe_num;
    bool pipe_to = true, pipe_from = true;
    if (pipenum != -1 && !numberpipe[userid][pipenum].ispiped) {
        // cout << "cmd: " << cmd.cmd << endl;
        if (pipe(numberpipe[userid][pipenum].pipe) < 0) {
            fprintf(stderr, "Error: Unable to create pipe. (%d)\n", pipenum);
            exit(EXIT_FAILURE);
        }
        numberpipe[userid][pipenum].ispiped = true;
    }

    /* Check user pipe receive */
    if (cmd.user_pipe_from > 0 && cmd.user_pipe_from < MAX_CLIENT_NUM) {
        if ((*clientset)[cmd.user_pipe_from].used && user_pipe[cmd.user_pipe_from][userid].ispiped == true) {
            // cout << "Section 1" << endl << flush;
            string pipe_from_msg = "*** " + (string)(*clientset)[userid].name + " (#" + to_string(userid)
                + ") just received from " + (*clientset)[cmd.user_pipe_from].name + " (#"
                + to_string(cmd.user_pipe_from) + ") by \'" + cmd.total_line + "\' ***\n";
            Broadcast(pipe_from_msg.c_str(), *clientset);
        } else if (!(*clientset)[cmd.user_pipe_from].used) {
            cerr << "*** Error: user #" << cmd.user_pipe_from << " does not exist yet. ***" << endl
                 << flush;
            pipe_from = false;
        } else if (user_pipe[cmd.user_pipe_from][userid].ispiped == false) {
            cerr << "*** Error: the pipe #" << cmd.user_pipe_from << "->#" << userid
                 << " does not exist yet. ***" << endl
                 << flush;
            pipe_from = false;
        }
    }

    /* Check user pipe send */
    if (cmd.user_pipe_to > 0 && cmd.user_pipe_to < MAX_CLIENT_NUM) {
        if ((*clientset)[cmd.user_pipe_to].used && user_pipe[userid][cmd.user_pipe_to].ispiped == false) {
            if (pipe(user_pipe[userid][cmd.user_pipe_to].pipe) < 0) {
                fprintf(stderr, "Error: Unable to create pipe. (%d)\n", pipenum);
                exit(EXIT_FAILURE);
            }
            user_pipe[userid][cmd.user_pipe_to].ispiped = true;
            string pipe_to_msg = "*** " + (string)(*clientset)[userid].name + " (#" + to_string(userid)
                + ") just piped \'" + cmd.total_line + "\' to " + (string)(*clientset)[cmd.user_pipe_to].name
                + " (#" + to_string(cmd.user_pipe_to) + ") ***\n";
            Broadcast(pipe_to_msg.c_str(), *clientset);
        } else if (!(*clientset)[cmd.user_pipe_to].used) {
            cerr << "*** Error: user #" << cmd.user_pipe_to << " does not exist yet. ***" << endl
                 << flush;
            pipe_to = false;
        } else if (user_pipe[userid][cmd.user_pipe_to].ispiped) {
            cerr << "*** Error: the pipe #" << userid << "->#" << cmd.user_pipe_to
                 << " already exists. ***" << endl
                 << flush;
            pipe_to = false;
        }
    }
    // if (cmd.user_pipe_from > 0)
    // cout << cmd.cmd << endl << user_pipe[cmd.user_pipe_from][userid].pipe[0] << " " << user_pipe[cmd.user_pipe_from][userid].pipe[1] << endl;
    signal(SIGCHLD, ChildHandler);
    while ((pid = fork()) < 0)
        usleep(1000);

    if (pid == 0) {
        // close((*clientset)[userid].sockfd);
        // child
        /* INPUT */
        // cout << "isnumberpipe: " << numberpipe[userid][count].ispiped << endl;

        if (numberpipe[userid][count].ispiped) {
            // Other process pipe to this process
            // cout << cmd.cmd << endl;
            close(numberpipe[userid][count].pipe[1]);
            dup2(numberpipe[userid][count].pipe[0], STDIN_FILENO);
            close(numberpipe[userid][count].pipe[0]);
            numberpipe[userid][count].ispiped = false;
        } else if (cmd.user_pipe_from > 0) {
            // cout << "user pipe from: " << cmd.user_pipe_from << " to: " << userid << endl << flush;
            // close(user_pipe[cmd.user_pipe_from][userid].pipe[1]);
            if (pipe_from == false)
                user_pipe[cmd.user_pipe_from][userid].pipe[0] = open("/dev/null", O_RDONLY);
            dup2(user_pipe[cmd.user_pipe_from][userid].pipe[0], STDIN_FILENO);
            close(user_pipe[cmd.user_pipe_from][userid].pipe[0]);
            // close(user_pipe[cmd.user_pipe_from][userid].pipe[1]);
            for (unsigned int i = 0; i < cmd.parameter.size(); i++) {
                if (cmd.parameter[i][0] == '<')
                    cmd.parameter.erase(cmd.parameter.begin() + i);
            }
        }

        /* OUTPUT */
        if (cmd.isExclamationmark) {
            dup2(numberpipe[userid][pipenum].pipe[1], STDERR_FILENO);
        }
        if (pipenum != -1 && numberpipe[userid][pipenum].ispiped) {
            /* pipe to other process */
            // close(numberpipe[userid][pipenum].pipe[0]);
            dup2(numberpipe[userid][pipenum].pipe[1], STDOUT_FILENO);
            close(numberpipe[userid][pipenum].pipe[1]);
        } else if (cmd.user_pipe_to > 0) {
            // cout << "user pipe to " << cmd.user_pipe_to << endl << flush;
            // close(user_pipe[userid][cmd.user_pipe_to].pipe[0]);
            if (pipe_to == false) {
                // cout << "123" << endl;
                int devnull = open("/dev/null", O_WRONLY);
                dup2(devnull, STDOUT_FILENO);
                close(devnull);
            } else {
                dup2(user_pipe[userid][cmd.user_pipe_to].pipe[1], STDOUT_FILENO);
                // dup2(fd, STDERR_FILENO);
                close(user_pipe[userid][cmd.user_pipe_to].pipe[1]);
            }
            for (unsigned int i = 0; i < cmd.parameter.size(); i++) {
                if (cmd.parameter[i][0] == '>')
                    cmd.parameter.erase(cmd.parameter.begin() + i);
            }
            // user_pipe[cmd.user_pipe_from][userid].ispiped = false;
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
        // close(numberpipe[userid][count].pipe[0]);
        // close(numberpipe[userid][count].pipe[1]);
        // client.number_pipe[count].ispiped = false;
        // for (unsigned int i = 0; i < NUMPIPE_SIZE; i++) {
        //     if (numberpipe[userid][i].ispiped)
        //         continue;
        //     close(numberpipe[userid][i].pipe[0]);
        //     close(numberpipe[userid][i].pipe[1]);
        // // }
        // for (unsigned int i = 0; i < MAX_CLIENT_NUM; i++) {
        // close(user_pipe[i][userid].pipe[1]);
        // close(user_pipe[i][userid].pipe[0]);
        // }
        // cout << arg[1] << endl;
        if (execvp(arg[0], arg) < 0)
            cerr << "Unknown command: [" + cmd.cmd + "]." << endl;

        exit(0);
    } else {
        usleep(1000);
        // parent
        if (numberpipe[userid][count].ispiped) {
            close(numberpipe[userid][count].pipe[1]);
            close(numberpipe[userid][count].pipe[0]);
            numberpipe[userid][count].ispiped = false;
        }
        if (cmd.user_pipe_to >= 0 && pipe_to == true) {
            /* when write to pipe */
            // close(user_pipe[userid][cmd.user_pipe_to].pipe[0]);
            close(user_pipe[userid][cmd.user_pipe_to].pipe[1]);
        }
        if (cmd.user_pipe_from >= 0 && pipe_from == true) {
            /* when read from pipe */
            close(user_pipe[cmd.user_pipe_from][userid].pipe[0]);
            // close(user_pipe[cmd.user_pipe_from][userid].pipe[1]);
            user_pipe[cmd.user_pipe_from][userid].ispiped = false;
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

void Broadcast(const char* msg, Client* clientset)
{
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (clientset[i].used) {
            write(clientset[i].sockfd, msg, strlen(msg));
        }
    }
}

void Welcome(int clientSocketfd)
{
    string str;
    str += "****************************************\n";
    str += "** Welcome to the information server. **\n";
    str += "****************************************\n";
    write(clientSocketfd, str.c_str(), strlen(str.c_str()));
}

void Logout(Client* clientset, int userid)
{
    string logoutmsg = "*** User \'" + (string)clientset[userid].name + "\' left. ***\n";
    Broadcast(logoutmsg.c_str(), clientset);
    memset(&clientset[userid], 0, sizeof(clientset[userid]));
}

void Who(Client* clientset, int userid)
{
    string str = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (clientset[i].used) {
            str += to_string(i) + "\t" + clientset[i].name + "\t" + clientset[i].addr
                + ":" + to_string(clientset[i].port) + "\t";
            if (userid == i)
                str += "<-me\n";
            else
                str += "\n";
        }
    }
    write(clientset[userid].sockfd, str.c_str(), strlen(str.c_str()));
}

void Name(Command cmd, Client* clientset, int userid)
{
    char name[20];
    memset(name, 0, 20);
    strncpy(name, cmd.parameter[0].c_str(), 20);
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (strncmp(name, clientset[i].name, 20) == 0) {
            cerr << "*** User \'" << name << "\' already exists. ***" << endl
                 << flush;
            return;
        }
    }
    strncpy(clientset[userid].name, name, 20);
    string changename;
    changename = "*** User from " + (string)clientset[userid].addr
        + ":" + to_string(clientset[userid].port) + " is named \'" + name + "\'. ***\n";
    Broadcast(changename.c_str(), clientset);
}

void Tell(Command cmd, Client* clientset, int userid)
{
    int target = stoi(cmd.parameter[0]);
    if (clientset[target].used == false) {
        cerr << "*** Error: user #" + cmd.parameter[0] + " does not exist yet. ***" << endl
             << flush;
    } else {
        string concat, msg;
        for (unsigned int i = 1; i < cmd.parameter.size(); i++)
            concat += cmd.parameter[i] + " ";
        concat.pop_back(); // delete the last blank.
        msg = "*** " + (string)clientset[userid].name + " told you ***: " + concat + "\n";
        write(clientset[target].sockfd, msg.c_str(), strlen(msg.c_str()));
    }
    return;
}

void Yell(Command cmd, Client* clientset, int userid)
{
    string concat = "";
    concat += "*** " + (string)clientset[userid].name + " yelled ***: ";

    for (unsigned int i = 0; i < cmd.parameter.size(); i++)
        concat += cmd.parameter[i] + " ";
    concat.pop_back(); // delete the last blank.
    concat += "\n";
    Broadcast(concat.c_str(), clientset);
}

void closepipe(NumberPipe** numberpipe, UserPipe** userpipe, int userid)
{
    for (int i = 0; i < NUMPIPE_SIZE; i++) {
        if (numberpipe[userid][i].ispiped) {
            close(numberpipe[userid][i].pipe[0]);
            close(numberpipe[userid][i].pipe[1]);
            numberpipe[userid][i].ispiped = false;
        }
    }
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (userpipe[userid][i].ispiped) {
            close(userpipe[userid][i].pipe[0]);
            close(userpipe[userid][i].pipe[1]);
            userpipe[userid][i].ispiped = false;
        }
        if (userpipe[i][userid].ispiped) {
            close(userpipe[i][userid].pipe[0]);
            close(userpipe[i][userid].pipe[1]);
            userpipe[i][userid].ispiped = false;
        }
    }
}

void Resetenv(map<string, string>* env, int userid)
{
    for (auto& it : env[userid]) {
        // cout << it.first << " " << it.second << endl;
        setenv(it.first.c_str(), it.second.c_str(), 1);
    }
}
