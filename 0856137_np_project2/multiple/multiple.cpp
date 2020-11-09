#include "shell.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace std;

static void Signalhandler(int signo);
static void ShareMem_Create();
static void Semaphore_Create();

Client* clientset;

int main(int argc, char* argv[])
{
    signal(SIGINT, Signalhandler);
    signal(SIGQUIT, Signalhandler);
    signal(SIGTERM, Signalhandler);
    signal(SIGUSR1, Signalhandler);
    signal(SIGCHLD, ChildHandler);
    ShareMem_Create();
    Semaphore_Create();
    clearenv();
    setenv("PATH", "bin:.", 1);
    // signal(SIGCHLD, ChildHandler);

    int serversocketfd = 0, clientSocketfd = 0;
    serversocketfd = socket(AF_INET, SOCK_STREAM, 0);

    // config
    struct sockaddr_in serverinfo, clientinfo;
    socklen_t addrlen = sizeof(clientinfo);
    bzero(&serverinfo, sizeof(serverinfo));
    serverinfo.sin_family = PF_INET;
    serverinfo.sin_addr.s_addr = INADDR_ANY;
    serverinfo.sin_port = htons(atoi(argv[1]));

    int flag = 1;
    if (setsockopt(serversocketfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        std::cerr << "!! REUSEADDR Flag Error !!" << std::endl;
    }
    if (setsockopt(serversocketfd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(int)) < 0) {
        std::cerr << "!! REUSEPORT Flag Error !!" << std::endl;
    }

    if (bind(serversocketfd, (struct sockaddr*)&serverinfo, sizeof(serverinfo)) < 0)
        cerr << "!! Bind ERROR !!" << endl;

    if (listen(serversocketfd, MAX_CLIENT_NUM) < 0)
        cerr << "!! Lensten ERROR !!" << endl;

    while (1) {
        clientSocketfd = accept(serversocketfd, (struct sockaddr*)&clientinfo, &addrlen);
        if (clientSocketfd < 0) {
            cerr << "!! Accept ERROR !!" << endl;
        }
        char addr[INET_ADDRSTRLEN];
        inet_ntop(clientinfo.sin_family, &(clientinfo.sin_addr), addr, INET_ADDRSTRLEN);

        cout << "accept:" << addr << endl;
        // cout << "port " << ClientInfo.sin_port << endl;
        cout << "port " << ntohs(clientinfo.sin_port) << endl;
        int childpid;

        if ((childpid = fork()) < 0)
            cerr << "!! Fork ERROR !!" << endl;
        if (childpid == 0) {
            // Child
            dup2(clientSocketfd, STDOUT_FILENO);
            dup2(clientSocketfd, STDERR_FILENO);
            dup2(clientSocketfd, STDIN_FILENO);
            // char* shell[1];
            // string str = "npshell";
            // shell[0] = strdup("npshell");
            // if(execvp(shell[0], shell) < 0)
            //     cerr << "!! Execute ERROR !!" << endl;
            Shell(clientinfo);
            // exit(0);
        } else {
            // parent
            close(clientSocketfd);
        }
    }
    return 0;
}

void Signalhandler(int signo)
{
    if (signo == SIGUSR1) {
        cerr << "Receive SIGUSR1" << endl;
        return;
    }
    if (signo == SIGINT || signo == SIGQUIT || signo == SIGTERM) {
        for (int i = 1; i < MAX_CLIENT_NUM; i++) {
            if (clientset[i].used)
                kill(clientset[i].pid, SIGINT);
        }
        shmdt(clientset);
        /* clear share memory */
        int sharemem_id = 0;
        if ((sharemem_id = shmget(SHAREMEM_KEY, MAX_CLIENT_NUM * sizeof(Client), FLAG)) < 0) {
            cerr << "!! shmget ERROR in Signalhandler:" << sharemem_id << endl;
            exit(1);
        }
        if (shmctl(sharemem_id, IPC_RMID, NULL) < 0) {
            cerr << "!! shmctl ERROR in Signalhandler" << endl;
            exit(1);
        }
        /* clear semaphore */
        int semaphore_id = -1;
        if ((semaphore_id = semget(SEMAPHORE_KEY, 1, FLAG)) < 0) {
            cerr << "!! semget ERROR in Signalhandler" << endl;
            exit(1);
        }
        union semun semaphore_union_del;
        if (semctl(semaphore_id, 0, IPC_RMID, semaphore_union_del) < 0) {
            cerr << "!! semctl ERROR in Signalhandler" << endl;
            exit(1);
        }
        /* clear user pipe */
        exit(0);
    }
}

void Semaphore_Create()
{
    int semaphore_id = -1;
    if ((semaphore_id = semget(SEMAPHORE_KEY, 1, FLAG | IPC_CREAT)) < 0) {
        cerr << "!! Semaphore ERROR" << endl;
        exit(1);
    }
    cout << "Semaphore id:" << semaphore_id << endl;
    union semun semaphore_union_init;
    semaphore_union_init.val = 1;
    if (semctl(semaphore_id, 0, SETVAL, semaphore_union_init) == -1) {
        cerr << "!! Semctl ERROR" << endl;
        exit(1);
    }
}

void ShareMem_Create()
{
    int sharemem_id = 1;
    clientset = nullptr;
    if ((sharemem_id = shmget(SHAREMEM_KEY,
             MAX_CLIENT_NUM * sizeof(Client), FLAG | IPC_CREAT))
        < 0) {
        cerr << "!! shemget ERROR, shmem_id=" << sharemem_id << endl;
        exit(1);
    }
    cout << sharemem_id << endl;
    if ((clientset = (Client*)shmat(sharemem_id, (Client*)0, 0)) < 0) {
        cerr << "!! shmat ERROR, share memory attach failed." << endl;
        exit(1);
    }
    memset(clientset, 0, MAX_CLIENT_NUM * sizeof(Client));
}
