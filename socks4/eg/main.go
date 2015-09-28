package main

import (
	"log"

	"github.com/v2e4lisp/sample/socks4"
)

func main() {
	s := &socks4.Server{}
	if err := s.Serve(":2345"); err != nil {
		log.Fatal(err)
	}
}
