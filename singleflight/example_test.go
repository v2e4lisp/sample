package singleflight_test

import (
        "fmt"
        "sync"
        "time"

        "github.com/v2e4lisp/sample/singleflight"
)

func Example() {
        hello := func(name interface{}) (interface{}, error) {
                msg := fmt.Sprint("hello: ", name)
                timer := time.NewTimer(time.Second * 1)
                <-timer.C
                fmt.Println("executing")
                return msg, nil
        }
        flight := singleflight.New(hello)

        var wg sync.WaitGroup
        t := func() {
                defer wg.Done()
                val, err := flight.Do("user")
                if err != nil {
                        fmt.Println(err)
                        return
                }
                if val, ok := val.(string); ok {
                        fmt.Println(val)
                        return
                }
                fmt.Println("failed")
        }
        wg.Add(3)
        go t()
        go t()
        go t()
        wg.Wait()
        // Output:
        // executing
        // hello: user
        // hello: user
        // hello: user
}
