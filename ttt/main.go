package main

/*
#include <termios.h>
#include <sys/ioctl.h>

struct termios tt;
struct winsize win;

void ttyFixWinsz(int slave) {
    ioctl(0, TIOCGWINSZ, (char *)&win);
    ioctl(slave, TIOCSWINSZ, (char *)&win);
}

void ttySave() {
    tcgetattr(0, &tt);
}

void ttyRestore() {
    tcsetattr(0, TCSAFLUSH, &tt);
}

void ttyRaw() {
    struct termios rtt;
    rtt = tt;
#if defined(SVR4)
    rtt.c_iflag = 0;
    rtt.c_lflag &= ~(ISIG|ICANON|XCASE|ECHO|ECHOE|ECHOK|ECHONL);
    rtt.c_oflag = OPOST;
    rtt.c_cc[VINTR] = CDEL;
    rtt.c_cc[VQUIT] = CDEL;
    rtt.c_cc[VERASE] = CDEL;
    rtt.c_cc[VKILL] = CDEL;
    rtt.c_cc[VEOF] = 1;
    rtt.c_cc[VEOL] = 0;
#else
    cfmakeraw(&rtt);
    rtt.c_lflag &= ~ECHO;
#endif
    tcsetattr(0, TCSAFLUSH, &rtt);
}

void ttyNoEcho() {
    struct termios rtt;
    rtt = tt;
    rtt.c_lflag &= ~(ICANON | ECHO | ECHONL);
    tcsetattr(0, TCSAFLUSH, &tt);
}
*/
import "C"
import (
        "encoding/gob"
        "flag"
        "io"
        "log"
        "os"
        "os/exec"
        "syscall"
        "time"

        "github.com/kr/pty"
)

func main() {
        action := flag.String("a", "record", "action name(record or replay)")
        flag.Parse()
        switch *action {
        case "record":
                record()
        case "replay":
                replay()
        default:
                flag.Usage()
        }
}

func record() {
        C.ttySave()
        defer C.ttyRestore()
        C.ttyRaw()

        master, slave, err := pty.Open()
        if err != nil {
                log.Fatalln("open:", err)
        }
        C.ttyFixWinsz(C.int(slave.Fd()))
        script, err := os.OpenFile("ttyrecord", os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0644)
        if err != nil {
                log.Fatalln(err)
        }

        sh := os.Getenv("SHELL")
        if sh == "" {
                sh = "/bin/sh"
        }
        attr := &syscall.SysProcAttr{
                Setsid:  true,
                Setctty: true,
                Ctty:    int(slave.Fd()),
        }
        shell := &exec.Cmd{
                Path:        sh,
                SysProcAttr: attr,
                Stdin:       slave,
                Stdout:      slave,
                Stderr:      slave,
        }
        go io.Copy(master, os.Stdin)
        go io.Copy(io.MultiWriter(newGobWriter(script), os.Stdout), master)
        shell.Run()
}

func replay() {
        C.ttySave()
        defer C.ttyRestore()
        C.ttyNoEcho()

        script, err := os.Open("ttyrecord")
        if err != nil {
                log.Fatalln(err)
        }

        var last time.Time
        dec := gob.NewDecoder(script)
        msg := &message{}
        for {
                if err := dec.Decode(msg); err != nil {
                        break
                }
                if !last.IsZero() {
                        wait := msg.T.Sub(last)
                        <-time.After(wait)
                }
                last = msg.T
                os.Stdout.Write(msg.P)
        }
}

type message struct {
        T time.Time
        P []byte
}

type gobWriter struct {
        *gob.Encoder
}

func newGobWriter(w io.Writer) *gobWriter {
        return &gobWriter{Encoder: gob.NewEncoder(w)}
}

func (gw *gobWriter) Write(p []byte) (n int, err error) {
        msg := message{
                T: time.Now(),
                P: p,
        }
        if err := gw.Encode(msg); err != nil {
                return 0, err
        }
        return len(p), nil
}
