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
