#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/event.h>


int tailf(char *path) {
        int fd;
        char buf[1024];
        int nb;

        int kq;
        struct kevent ev;

        fd = open(path, 0, O_RDONLY);
        if (fd < 0) {
                perror("open");
                return -1;
        }

        kq = kqueue();
        if (kq < 0) {
                perror("kqueue");
                return -1;
        }

        EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
        kevent(kq, &ev, 1, NULL, 0, NULL);

        while (1) {
                nb = read(fd, buf, 1023);

                // error
                if (nb < 0) {
                        perror("read");
                        return -1;
                }

                // normal
                if (nb > 0) {
                        buf[nb] = 0;
                        printf("%s", buf);
                        continue;
                }

                // EOF
                if (kevent(kq, NULL, 0,  &ev, 1, NULL) < 0) {
                        perror("kevent");
                        return -1;
                }

                if (ev.flags & EV_ERROR) {
                        fprintf(stderr, "kevent: %s", strerror(ev.data));
                        return -1;
                }
        }

        return 0;
}


int main(int argc, char **argv) {
        if (argc != 2) {
                printf("tailf file\n");
                return -1;
        }

        return tailf(argv[1]);
}
