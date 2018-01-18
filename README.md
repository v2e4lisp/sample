unicode NFC / NFD
------------------

```go
package main

import (
	"fmt"

	"golang.org/x/text/unicode/norm"
)

// > go run /tmp/unicode.go
// single accent NFC: é
// "\u00e9"
//
// single accent NFD: é
// "e\u0301"
//
// mutli accent NFD: é́
// "e\u0301\u0301"
//
// mutli accent NFC: é́
// "\u00e9\u0301"
func main() {
	fmt.Print("single accent NFC: ")
	e := "\u00e9"
	fmt.Printf("%s\n", e)
	fmt.Printf("%+q\n", e)

	fmt.Print("\nsingle accent NFD: ")
	e2 := norm.NFD.String(e)
	fmt.Printf("%s\n", e2)
	fmt.Printf("%+q\n", e2)

	fmt.Print("\nmutli accent NFD: ")
	e3 := norm.NFD.String("e\u0301\u0301")
	fmt.Printf("%s\n", e3)
	fmt.Printf("%+q\n", e3)

	fmt.Print("\nmutli accent NFC: ")
	e4 := norm.NFC.String(e3)
	fmt.Printf("%s\n", e4)
	fmt.Printf("%+q\n", e4)
}
```

tcp server local addr
---------------------

```go
package main

import (
	"log"
	"net"
)

// Client
//
// ➜  ss nc 127.0.0.1 10080
// ➜  ss nc 127.0.0.1 10080
// ➜  ss nc 127.0.0.1 10080
// ➜  ss nc 127.0.0.1 10080
// ➜  ss nc 127.0.0.1 10080

// Server
//
// ➜  ss go run /tmp/port.go
// 2018/01/16 23:15:39 127.0.0.1:10080 -> 127.0.0.1:59513
// 2018/01/16 23:15:40 127.0.0.1:10080 -> 127.0.0.1:59514
// 2018/01/16 23:15:41 127.0.0.1:10080 -> 127.0.0.1:59515
// 2018/01/16 23:15:42 127.0.0.1:10080 -> 127.0.0.1:59517
// 2018/01/16 23:15:42 127.0.0.1:10080 -> 127.0.0.1:59518

func main() {
	ln, err := net.Listen("tcp", ":10080")
	if err != nil {
		log.Fatal(err)
	}
	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Fatal(err)
		}
		go func(conn net.Conn) {
			log.Println(conn.LocalAddr(), "->", conn.RemoteAddr())
			conn.Close()
		}(conn)
	}
}

```

tcpdump
--------

```sh
sudo tcpdump -A -s0 -ilo0 src port 9000
sudo tcpdump -A -s0 -ilo0 dst port 9000
```
