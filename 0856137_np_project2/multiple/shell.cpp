#include "shell.h"
// #include "message.h"

static Client* clientset;
static void Clienthandler(int signo);
static int sharemem_id = 0, semaphore_id = 0, userid = -1;
static int Semaphore_P();
static int Semaphore_V();

void Shell(sockaddr_in clientinfo)
{
    signal(SIGUSR1, Clienthandler);
    signal(SIGUSR2, Clienthandler);
    signal(SIGINT, Clienthandler);
    signal(SIGQUIT, Clienthandler);
    signal(SIGTERM, Clienthandler);
    Welcome();
    InitialClient(clientinfo);
    Login();
    int commamdcount = 0;
    bool Needtowait = false;
    pid_t lastpid;
    string input;
    NumberPipe Number_pipe[NUMPIPE_SIZE];

    queue<Command> command_que;
    queue<string> input_cmd, Path_que;
    setenv("PATH", "bin:.", 1);

    // cout << "dasda" << endl;
    while (cout << "% " << flush && getline(cin, input)) {
        // cout.flush();
        // input.clear();
        // recv(clientSocketfd, inputBuffer, COMMANDLEN, 0);
        ClearQueue(input_cmd);
        input_cmd = SplitCommand(input);
        while (!input_cmd.empty()) {
            /* command sonfig */
            commamdcount++;
            commamdcount %= NUMPIPE_SIZE;
            Command cmd;
            // memset(&cmd, 0, sizeof(Command));
            // cout << input.size() << endl;
            cmd.total_line = input;
            cmd.parameter.clear();
            // cmd.total_line.pop_back();          // pop \r or \n
            if (cmd.total_line.back() == '\r' || cmd.total_line.back() == '\n')
                cmd.total_line.pop_back();
            cmd.cmd = input_cmd.front();
            input_cmd.pop();
            cmd.options = ResolveCommands(Path_que, cmd.cmd);
            cmd.cmd_cnt = commamdcount;
            cmd.pipe_num = -1;
            cmd.user_pipe_from = -1;
            cmd.user_pipe_to = -1;
            cmd.isExclamationmark = false;
            cmd.isRedirect = false;

            /* NOTE: parameter do NOT include command. */
            ParseCommand(cmd, input_cmd);
            command_que.push(cmd);
        }
        while (!command_que.empty()) {
            Command cmd = command_que.front();
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
                if (getenv(cmd.parameter[0].c_str()))
                    cout << getenv(cmd.parameter[0].c_str()) << endl;
                break;
            case who:
                Who();
                break;
            case name:
                Name(cmd);
                break;
            case tell:
                Tell(cmd);
                break;
            case yell:
                Yell(cmd);
                break;
            case EXIT:
                Logout();
                break;
            case EXEC:
                ExecuteCommand(cmd, Number_pipe, lastpid);
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
    }

    return;
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
#if 1
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
#endif
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

inline bool IsExecutionCmd(CommandOptions cmdtype)
{
    return (cmdtype == EXEC);
}

void ExecuteCommand(Command cmd, NumberPipe* Number_pipe, pid_t& pid)
{
    int count = cmd.cmd_cnt, pipenum = cmd.pipe_num, userpipe_writefd;
    string fifopath = "";
    bool pipe_to = true, pipe_from = true;
    /* Set pipe target */
    if (pipenum != -1 && !Number_pipe[pipenum].ispiped) {
        if (pipe(Number_pipe[pipenum].pipe) < 0) {
            fprintf(stderr, "Error: Unable to create pipe. (%d)\n", pipenum);
            exit(EXIT_FAILURE);
        }
        Number_pipe[pipenum].ispiped = true;
    }

    /* Check user pipe receive */
    if (cmd.user_pipe_from > 0 && cmd.user_pipe_from < MAX_CLIENT_NUM) {
        if (Semaphore_P() < 0) {
            cerr << "!! Get Semaphore failed." << endl;
            exit(1);
        }
        /********** Get semaphore **********/
        if (clientset[cmd.user_pipe_from].used && clientset[userid].user_pipe_from[cmd.user_pipe_from] == 0) {
            // cout << "Section 1" << endl << flush;
            string pipe_from_msg = "*** " + (string)clientset[userid].name + " (#" + to_string(userid)
                + ") just received from " + clientset[cmd.user_pipe_from].name + " (#"
                + to_string(clientset[cmd.user_pipe_from].id) + ") by \'" + cmd.total_line + "\' ***";
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
            Broadcast(pipe_from_msg.c_str());
        } else if (!clientset[cmd.user_pipe_from].used) {
            cerr << "*** Error: user #" << cmd.user_pipe_from << " does not exist yet. ***" << endl
                 << flush;
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
            // close(fd);
            // return;'
            pipe_from = false;
        } else if (clientset[userid].user_pipe_from[cmd.user_pipe_from] == -1) {
            cerr << "*** Error: the pipe #" << cmd.user_pipe_from << "->#" << userid
                 << " does not exist yet. ***" << endl
                 << flush;
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
            // close(fd);
            // return;
            pipe_from = false;
        }
    }

    /* Check user pipe send */
    if (cmd.user_pipe_to > 0 && cmd.user_pipe_to < MAX_CLIENT_NUM) {
        if (Semaphore_P() < 0) {
            cerr << "!! Get Semaphore failed." << endl;
            exit(1);
        }
        /********** Get semaphore **********/
        if (clientset[cmd.user_pipe_to].used && clientset[cmd.user_pipe_to].user_pipe_from[userid] == -1) {
            string pipe_to_msg = "*** " + (string)clientset[userid].name + " (#" + to_string(userid)
                + ") just piped \'" + cmd.total_line + "\' to " + (string)clientset[cmd.user_pipe_to].name
                + " (#" + to_string(cmd.user_pipe_to) + ") ***";
            pid_t target = clientset[cmd.user_pipe_to].pid;
            clientset[cmd.user_pipe_to].user_pipe_from[userid] = 1;
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
            // cout << "pipe_to_msg: " << pipe_to_msg << endl << flush;
            /* Create fifo */
            fifopath = "./user_pipe/" + to_string(userid) + "to" + to_string(cmd.user_pipe_to);
            mkfifo(fifopath.c_str(), FLAG);
            kill(target, SIGUSR2);

            userpipe_writefd = open(fifopath.c_str(), O_WRONLY);
            Broadcast(pipe_to_msg.c_str());
        } else if (!clientset[cmd.user_pipe_to].used) {
            cerr << "*** Error: user #" << cmd.user_pipe_to << " does not exist yet. ***" << endl
                 << flush;
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
            // close(fd);
            // return;
            pipe_to = false;
        } else if (clientset[cmd.user_pipe_to].user_pipe_from[userid] >= 0) {
            cerr << "*** Error: the pipe #" << userid << "->#" << cmd.user_pipe_to
                 << " already exists. ***" << endl
                 << flush;
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
            // close(fd);
            // return;
            pipe_to = false;
        }
    }

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

        if (cmd.user_pipe_from > 0) {
            // cout << "user pipe from " << cmd.user_pipe_from << endl << flush;
            if (Semaphore_P() < 0) {
                cerr << "!! Get Semaphore failed." << endl;
                exit(1);
            }
            /********** Get semaphore **********/
            if (pipe_from == false) {
                int devnull = open("/dev/null", O_RDONLY);
                clientset[userid].fd_table[cmd.user_pipe_from] = devnull;
            }
            //if (clientset[userid].user_pipe_from[cmd.user_pipe_from] == 0) {
            dup2(clientset[userid].fd_table[cmd.user_pipe_from], STDIN_FILENO);
            // }
            close(clientset[userid].fd_table[cmd.user_pipe_from]);
            clientset[userid].fd_table[cmd.user_pipe_from] = -1;
            clientset[userid].user_pipe_from[cmd.user_pipe_from] = -1;
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
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
        } else if (cmd.user_pipe_to > 0) {
            if (pipe_to == false) {
                userpipe_writefd = open("/dev/null", O_WRONLY);
            }
            // cout << "user pipe to " << cmd.user_pipe_to << endl << flush;
            dup2(userpipe_writefd, STDOUT_FILENO);
            // dup2(fd, STDERR_FILENO);
            close(userpipe_writefd);
            remove(fifopath.c_str()); // remove file, but fifo is still work.
        }

        /* Redirect to file */
        if (cmd.isRedirect) {
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
        // cout << arg[1] << endl;
        if (execvp(arg[0], arg) < 0)
            cerr << "Unknown command: [" + cmd.cmd + "]." << endl
                 << flush;
        exit(0);
    } else {
        /* parent */
        if (Semaphore_P() < 0) {
            cerr << "!! Get Semaphore failed." << endl;
            exit(1);
        }
        /********** Get semaphore **********/
        if (cmd.user_pipe_from >= 0 && pipe_from == true)
            close(clientset[userid].fd_table[cmd.user_pipe_from]);
        // clientset[userid].fd_table[cmd.user_pipe_from] = -1;
        /********** Release semaphore **********/
        if (Semaphore_V() < 0) {
            cerr << "!! Release semaphore failed." << endl;
            exit(1);
        }
        if (cmd.user_pipe_to >= 0 && pipe_to == true)
            close(userpipe_writefd);
        if (Number_pipe[count].ispiped) {
            close(Number_pipe[count].pipe[1]);
            close(Number_pipe[count].pipe[0]);
            Number_pipe[count].ispiped = false;
        }
    }
    return;
}

inline void ClearQueue(queue<string>& que)
{
    while (!que.empty())
        que.pop();
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

void Clienthandler(int signo)
{
    // signal(signo, SIG_IGN);
    /* Receive massage */
    if (signo == SIGUSR1) {
        // cout << "Receive SIGUSR1" << endl << flush;
        if (Semaphore_P() < 0) {
            cerr << "!! Get Semaphore failed." << endl;
            exit(1);
        }
        /********** Get semaphore **********/
        /* Write message to socket fd */
        if (strlen(clientset[userid].msg) > 0) {
            // cout << "write" << endl << flush;
            write(STDOUT_FILENO, clientset[userid].msg, strlen(clientset[userid].msg));
            cout << endl
                 << flush;
            memset(clientset[userid].msg, 0, MAX_MASSAGE_SIZE);
        }
        /********** Release semaphore **********/
        if (Semaphore_V() < 0) {
            cerr << "!! Release semaphore failed." << endl;
        }
    } else if (signo == SIGUSR2) {
        // cout << "In SIGUSR2" << endl << flush;
        if (Semaphore_P() < 0) {
            cerr << "!! Get Semaphore failed." << endl;
            exit(1);
        }
        /********** Get semaphore **********/
        /* Using for user pipe open */
        for (int i = 1; i < MAX_CLIENT_NUM; i++) {
            if (clientset[userid].user_pipe_from[i] == 1) {
                string fifopath = "./user_pipe/" + to_string(i) + "to" + to_string(userid);
                // cout << "read :" << mkfifo(fifopath.c_str(), FLAG);
                clientset[userid].fd_table[i] = open(fifopath.c_str(), O_RDONLY);
                clientset[userid].user_pipe_from[i] = 0;
                // cout << fifopath << endl << flush;
                // break;
            }
        }
        /********** Release semaphore **********/
        if (Semaphore_V() < 0) {
            cerr << "!! Release semaphore failed." << endl;
        }
        // cout << "Out SIGUSR2" << endl << flush;
    } else if (signo == SIGINT || signo == SIGQUIT || signo == SIGTERM) {
        union semun sem_union;
        if (semctl(semaphore_id, 0, IPC_RMID, sem_union) < 0) {
            cerr << "!! semctl ERROR" << endl;
            exit(1);
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        Logout();
    }
    // signal(signo, Clienthandler);
}

void InitialClient(sockaddr_in clientinfo)
{
    if ((semaphore_id = semget(SEMAPHORE_KEY, 1, FLAG)) < 0) {
        cerr << "!! semget ERROR in initial" << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    /* Get share memory */
    if ((sharemem_id = shmget(SHAREMEM_KEY, MAX_CLIENT_NUM * sizeof(Client), FLAG)) < 0) {
        cerr << "! ! Client shmget ERROR" << endl;
        exit(1);
    }
    if ((clientset = (Client*)shmat(sharemem_id, (Client*)0, 0)) == (Client*)-1) {
        cerr << "!! Client shmat ERROR" << endl;
        exit(1);
    }
    if (Semaphore_P() < 0) {
        cerr << "!! Get Semaphore failed." << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    /* Set client info */
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (clientset[i].used == false) {
            char addr[INET_ADDRSTRLEN];
            inet_ntop(clientinfo.sin_family, &(clientinfo.sin_addr), addr, INET_ADDRSTRLEN);
            // myinfo = clientset[i];
            userid = i;
            clientset[i].id = i;
            clientset[i].used = true;
            clientset[i].pid = getpid();
            clientset[i].port = ntohs(clientinfo.sin_port);
            strncpy(clientset[i].addr, addr, INET_ADDRSTRLEN);
            strncpy(clientset[i].name, "(no name)", 20);
            for (int j = 1; j < MAX_CLIENT_NUM; j++) {
                clientset[userid].user_pipe_from[j] = -1;
                clientset[userid].fd_table[j] = -1;
            }
            // cout << clientset[i].pid << endl;
            // clientset[i].name = {("\'(no name)\'").c_str()};
            break;
        }
    }
    /********** Release semaphore **********/
    if (Semaphore_V() < 0) {
        cerr << "!! Release semaphore failed." << endl;
        exit(1);
    }
}

void Broadcast(const char* msg)
{
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (Semaphore_P() < 0) {
            cerr << "!! Get Semaphore failed." << endl;
            exit(1);
        }
        /********** Get semaphore **********/
        // cout << i << " " << clientset[i].used << endl;
        if (clientset[i].used) {
            strncpy(clientset[i].msg, msg, strlen(msg));
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
            /* What if process no longer exixst? */
            kill(clientset[i].pid, SIGUSR1);
            // cout << "send to pid: " << clientset[i].pid << endl;

        } else {
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl;
                exit(1);
            }
        }
    }
    usleep(500);
}

int Semaphore_P()       // Probeer (try)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(semaphore_id, &sem_b, 1) < 0) {
        cerr << "!! semaphore operation" << endl;
        return -1;
    }
    return 0;
}

int Semaphore_V()       // Verhoog (increment)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(semaphore_id, &sem_b, 1) < 0) {
        cerr << "!! semaphore operation" << endl;
        return -1;
    }
    return 0;
}

void Login()
{
    if (Semaphore_P() < 0) {
        cerr << "!! Get Semaphore failed." << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    string str;
    str = "*** User \'" + (string)clientset[userid].name + "\' entered from " + clientset[userid].addr
        + ":" + to_string(clientset[userid].port) + ". ***";
    /********** Release semaphore **********/
    if (Semaphore_V() < 0) {
        cerr << "!! Release semaphore failed." << endl;
        exit(1);
    }
    Broadcast(str.c_str());
    return;
}

void Logout()
{
    if (Semaphore_P() < 0) {
        cerr << "!! Get Semaphore failed." << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    // for (int i = 1; i < MAX_CLIENT_NUM; i++) {
    //     if (clientset[userid].fd_table[i] != -1)
    //         close(clientset[userid].fd_table[i]);
    // }
    string logoutmsg = "*** User \'" + (string)clientset[userid].name + "\' left. ***";
    /********** Release semaphore **********/
    if (Semaphore_V() < 0) {
        cerr << "!! Release semaphore failed." << endl;
        exit(1);
    }
    Broadcast(logoutmsg.c_str());
    memset(&clientset[userid], 0, sizeof(clientset[userid]));
    shmdt(clientset);
    exit(0);
}

void Welcome()
{
    cout << "****************************************" << endl
         << "** Welcome to the information server. **" << endl
         << "****************************************" << endl;
    return;
}

void Who()
{
    string str = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
    Client tmp;
    if (Semaphore_P() < 0) {
        cerr << "!! Get Semaphore failed." << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (clientset[i].used) {
            tmp = clientset[i];
            str += to_string(i) + "\t" + tmp.name + "\t" + tmp.addr + ":" + to_string(tmp.port) + "\t";
            if (userid == i)
                str += "<-me\n";
            else
                str += "\n";
        }
    }
    /********** Release semaphore **********/
    if (Semaphore_V() < 0) {
        cerr << "!! Release semaphore failed." << endl;
        exit(1);
    }
    cout << str << flush;
    return;
}

void Name(Command cmd)
{
    char name[20];
    memset(name, 0, 20);
    strncpy(name, cmd.parameter[0].c_str(), 20);
    if (Semaphore_P() < 0) {
        cerr << "!! Get Semaphore failed." << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    for (int i = 1; i < MAX_CLIENT_NUM; i++) {
        if (strncmp(name, clientset[i].name, 20) == 0) {
            cerr << "*** User \'" << name << "\' already exists. ***" << endl
                 << flush;
            /********** Release semaphore **********/
            if (Semaphore_V() < 0) {
                cerr << "!! Release semaphore failed." << endl
                     << flush;
                exit(1);
            }
            return;
        }
    }
    strncpy(clientset[userid].name, name, 20);
    string changename;
    changename = "*** User from " + (string)clientset[userid].addr
        + ":" + to_string(clientset[userid].port) + " is named \'" + name + "\'. ***";
    /********** Release semaphore **********/
    if (Semaphore_V() < 0) {
        cerr << "!! Release semaphore failed." << endl;
        exit(1);
    }
    Broadcast(changename.c_str());
}

void Tell(Command cmd)
{
    int target = stoi(cmd.parameter[0]);
    if (Semaphore_P() < 0) {
        cerr << "!! Get Semaphore failed." << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    if (clientset[target].used == false) {
        cerr << "*** Error: user #" + cmd.parameter[0] + " does not exist yet. ***" << endl
             << flush;
        /********** Release semaphore **********/
        if (Semaphore_V() < 0) {
            cerr << "!! Release semaphore failed." << endl;
            exit(1);
        }
    } else {
        string concat = "", msg;
        for (unsigned int i = 1; i < cmd.parameter.size(); i++)
            concat += cmd.parameter[i] + " ";
        concat.pop_back(); // delete the last blank.
        msg = "*** " + (string)clientset[userid].name + " told you ***: " + concat;
        strncpy(clientset[target].msg, msg.c_str(), strlen(msg.c_str()));
        /********** Release semaphore **********/
        if (Semaphore_V() < 0) {
            cerr << "!! Release semaphore failed." << endl;
            exit(1);
        }
        // cout << clientset[target].pid << endl;
        kill(clientset[target].pid, SIGUSR1);
    }
    return;
}

void Yell(Command cmd)
{
    string concat = "";
    if (Semaphore_P() < 0) {
        cerr << "!! Get Semaphore failed." << endl;
        exit(1);
    }
    /********** Get semaphore **********/
    concat += "*** " + (string)clientset[userid].name + " yelled ***: ";
    /********** Release semaphore **********/
    if (Semaphore_V() < 0) {
        cerr << "!! Release semaphore failed." << endl;
        exit(1);
    }

    for (unsigned int i = 0; i < cmd.parameter.size(); i++)
        concat += cmd.parameter[i] + " ";
    concat.pop_back(); // delete the last blank.
    Broadcast(concat.c_str());
}
