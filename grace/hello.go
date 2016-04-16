package main

import (
        "fmt"
        "log"
        "net"
        "net/http"
        "os"
        "strconv"
        "time"
)

type tcpKeepAliveListener struct {
        *net.TCPListener
}

func (ln tcpKeepAliveListener) Accept() (c net.Conn, err error) {
        tc, err := ln.AcceptTCP()
        if err != nil {
                return
        }
        tc.SetKeepAlive(true)
        tc.SetKeepAlivePeriod(3 * time.Minute)
        return tc, nil
}

func main() {
        w := os.Getenv("GRACE_READY_FD")
        l := os.Getenv("GRACE_SOCKET_FD")

        wfd, err := strconv.Atoi(w)
        if err != nil {
                log.Fatalln(err)
        }
        lfd, err := strconv.Atoi(l)
        if err != nil {
                log.Fatalln(err)
        }
        wpipe := os.NewFile(uintptr(wfd), "ready_pipe")
        lfile := os.NewFile(uintptr(lfd), "listener")
        ln, err := net.FileListener(lfile)
        if err != nil {
                log.Fatalln(err)
        }

        http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
                fmt.Fprintf(w, "Hello World!")
        })

        go func() {
                <-time.After(1 * time.Second)
                wpipe.Write([]byte{1})
        }()

        err = http.Serve(tcpKeepAliveListener{ln.(*net.TCPListener)}, nil)
        if err != nil {
                log.Fatalln(err)
        }
}
