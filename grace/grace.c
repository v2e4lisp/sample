#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define DEFAULT_BACKLOG 100000
#define DEFAULT_START_TIMEOUT 10
#define SYSERR(msg) perror(msg);exit(1)
#define USRERR(msg) fprintf(stderr, msg);exit(1)
#define GRACE_SOCKET_FD "GRACE_SOCKET_FD"
#define GRACE_READY_FD "GRACE_READY_FD"
#define GRACE_BACKLOG "GRACE_BACKLOG"
#define GRACE_START_TIMEOUT "GRACE_START_TIMEOUT"


struct timeval timeout; // wait child timeout;

int childpid = 0;
int termniate = 0;
sigset_t osigset;

char sighupno = '1';
char sigchldno = '2';
char sigtermno = '3';
int srd;
int swr;

void sighup_handler(int signo) {
    write(swr, &sighupno, 1);
}

void sigchld_handler(int signo) {
    write(swr, &sigchldno, 1);
}

void sigterm_handler(int signo) {
    write(swr, &sigtermno, 1);
}

void wait_restart() {
    int restart;
    char signos[16];
    int pid;
    int nb;
    int i;

    while (1) {
        restart = 0;
        pause();

        while(1) {
            nb = read(srd, signos, 16);
            if (nb < 0 && errno == EAGAIN) {
                break;
            }
            for (i = 0; i < nb; i++) {
                if (signos[i] == sigtermno) {
                    termniate = 1;
                }
                if (signos[i] == sighupno) {
                    restart = 1;
                }
            }
        }

        while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
            if (pid == childpid) {
                childpid = 0;
                termniate = 1;
            }
        }

        if (restart || termniate) {
            return;
        }
    }
}

void wait_child(int pid, int rd) {
    int nfd;
    int nb;
    char ph[1];
    fd_set rset;

 SELECT:
    FD_ZERO(&rset);
    FD_SET(rd, &rset);
    nfd = select(rd + 1, &rset, NULL, NULL, &timeout);
    if (nfd == -1 && errno == EINTR) {
        goto SELECT;
    }

    if (!FD_ISSET(rd, &rset)) {
        kill(pid, SIGTERM);
        USRERR("timeout; child does not finish yet\n");
    }

 READ:
    nb = read(rd, ph, 1);
    if (nb == -1 && errno == EINTR) {
        goto READ;
    }

    if (nb == -1) {
        SYSERR("read from pipe");
    }
    if (nb == 0) {
        USRERR("EOF detected; child failed to start\n");
    }
}

void setup_signals() {
    struct sigaction sachld;
    struct sigaction sahup;
    struct sigaction saterm;
    sigset_t sigset;
    int spipe[2];
    // int flag;

    // setup pipe for signal handling
    if (pipe(spipe) < 0) {
        SYSERR("pipe");
    }
    srd = spipe[0];
    swr = spipe[1];

    // non blocking
    // flag = fcntl(srd, F_GETFL);
    if (fcntl(srd, F_SETFL, O_NONBLOCK) == -1) {
        SYSERR("fcntl O_NONBLOCK");
    }
    // flag = fcntl(swr, F_GETFL);
    if (fcntl(swr, F_SETFL, O_NONBLOCK) == -1) {
        SYSERR("fcntl O_NONBLOCK");
    }

    // only SIGHUP, SIGCHLD or SIGTERM are allowed
    sigfillset(&sigset);
    sigdelset(&sigset, SIGHUP);
    sigdelset(&sigset, SIGCHLD);
    sigdelset(&sigset, SIGTERM);
    sigdelset(&sigset, SIGINT);
    sigprocmask(SIG_SETMASK, &sigset, &osigset);

    sachld.sa_handler = sigchld_handler;
    sigfillset(&sachld.sa_mask);
    sachld.sa_flags = 0;
    sigaction(SIGCHLD, &sachld, NULL);

    sahup.sa_handler = sighup_handler;
    sigfillset(&sahup.sa_mask);
    sahup.sa_flags = 0;
    sigaction(SIGHUP, &sahup, NULL);

    saterm.sa_handler = sigterm_handler;
    sigfillset(&saterm.sa_mask);
    saterm.sa_flags = 0;
    sigaction(SIGTERM, &saterm, NULL);
}

void clear_sigmask() {
    sigprocmask(SIG_SETMASK, &osigset, NULL);
}

void setenvs(int sockfd, int wr) {
    char sbuf[10];
    char wbuf[10];
    sprintf(sbuf, "%d", sockfd);
    sprintf(wbuf, "%d", wr);
    setenv(GRACE_SOCKET_FD, sbuf, 1);
    setenv(GRACE_READY_FD, wbuf, 1);
}

int createsock(int port) {
    int sockfd;
    struct sockaddr_in saddr;
    int backlog;
    int reuse = 1;
    char *p;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        SYSERR("socket");
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        SYSERR("setsockopt");
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
        SYSERR("bind");
    }

    backlog = DEFAULT_BACKLOG;
    if ((p = getenv(GRACE_BACKLOG)) != NULL) {
        backlog = atoi(p);
    }
    if (listen(sockfd, backlog) < 0) {
        SYSERR("listen");
    }

    return sockfd;
}

void loop(int port, char *cmd[]) {
    int sockfd; // socket fd
    int ready[2]; // ready pipe
    int rd; // pipe reader fd
    int wr; // pipe writer fd
    int pid; // temp child pid;
    fd_set rset; // fd set for pipe reader
    char *p; // GRACE_START_TIMEOUT
    int rt; // start timeout

    sockfd = createsock(port);
    setup_signals();

    rt = 10;
    if ((p = getenv(GRACE_START_TIMEOUT)) != NULL) {
        rt = atoi(p);
    }
    timeout.tv_sec = rt;
    timeout.tv_usec = 0;

    while(1) {
        if (pipe(ready) < 0) {
            SYSERR("pipe");
        }
        rd = ready[0];
        wr = ready[1];

        pid = fork();
        if (pid < 0) {
            SYSERR("fork");
        }

        if (pid == 0) {
            close(rd);
            close(srd);
            close(swr);
            // sigmask is preserved for child process, so we need to clear it;
            clear_sigmask();
            setenvs(sockfd, wr);
            execvp(cmd[0], cmd);
            SYSERR("execvp");
        }

        // close the write end of the pipe;
        close(wr);

        wait_child(pid, rd);

        close(rd);

        // successfully started; then kill the old one.
        if (childpid) {
            kill(childpid, SIGTERM);
        }
        childpid = pid;

        wait_restart();

        if (termniate) {
            if (childpid) {
                kill(childpid, SIGTERM);
            }
            exit(0);
        }
    }
}

/**
 * grace [port] [cmd]
 */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        USRERR("grace [port] [cmd]\n");
    }

    int port = atoi(argv[1]);
    loop(port, &argv[2]);
}
