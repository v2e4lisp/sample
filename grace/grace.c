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

#define DEFAULT_BACKLOG 100000
#define DEFAULT_START_TIMEOUT 10
#define SYSERR(msg) perror(msg);exit(1)
#define USRERR(msg) fprintf(stderr, msg);exit(1)
#define GRACE_SOCKET_FD "GRACE_SOCKET_FD"
#define GRACE_READY_FD "GRACE_READY_FD"
#define GRACE_BACKLOG "GRACE_BACKLOG"
#define GRACE_START_TIMEOUT "GRACE_START_TIMEOUT"

int childpid = 0;
int restart = 0;
sigset_t osigset;

void sigterm_handler(int signo) {
    restart = 1;
}

void sigchld_handler(int signo) {
    int pid = 0;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        if (pid == childpid) {
            childpid = 0;
        }
    }
}

void setup_signals() {
    struct sigaction sachld;
    struct sigaction saterm;
    sigset_t sigset;

    sigfillset(&sigset);
    sigdelset(&sigset, SIGTERM);
    sigdelset(&sigset, SIGCHLD);
    sigprocmask(SIG_SETMASK, &sigset, &osigset);

    sachld.sa_handler = sigchld_handler;
    sigfillset(&sachld.sa_mask);
    sachld.sa_flags = 0;
    sigaction(SIGCHLD, &sachld, NULL);

    saterm.sa_handler = sigterm_handler;
    sigfillset(&saterm.sa_mask);
    saterm.sa_flags = 0;
    sigaction(SIGTERM, &saterm, NULL);
}

void clear_sigmask() {
    sigprocmask(SIG_SETMASK, &osigset, NULL);
}

void wait_restart() {
    while(1) {
        pause();
        if (!childpid || restart) {
            restart = 0;
            break;
        }
    }
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
    int nb; // number of bytes read from pipe
    char ph[1]; // placeholder for the byte read from pipe
    struct timeval timeout; // select timeout;
    fd_set rset; // fd set for pipe reader
    char *p; // GRACE_START_TIMEOUT
    int rt; // start timeout

    setup_signals();
    sockfd = createsock(port);
    rt = 10;
    if ((p = getenv(GRACE_START_TIMEOUT)) != NULL) {
        rt = atoi(p);
    }
    timeout.tv_sec = rt;
    timeout.tv_usec = 0;

    while(1) {
        if (pipe(ready) != 0) {
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
            // sigmask is preserved for child process, so we need to clear it;
            clear_sigmask();
            setenvs(sockfd, wr);
            execvp(cmd[0], cmd);
            SYSERR("execvp");
        }

        // close the write end of the pipe;
        close(wr);

        // wait for child to finish starting;
        FD_ZERO(&rset);
        FD_SET(rd, &rset);
        select(rd + 1, &rset, NULL, NULL, &timeout);

        // timeout; child does not finish yet;
        if (!FD_ISSET(rd, &rset)) {
            USRERR("timeout\n");
        }

        nb = read(rd, ph, 1);
        if (nb < 0) {
            SYSERR("read from pipe");
        }
        // pipe is closed; child failed to start;
        if (nb == 0) {
            USRERR("child failed to start\n");
        }

        close(rd);

        // successfully started; then kill the old one.
        if (childpid) {
            kill(childpid, SIGTERM);
        }
        childpid = pid;

        wait_restart();
        if (!childpid) {
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
