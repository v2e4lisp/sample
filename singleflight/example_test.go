package singleflight_test

import (
        "errors"
        "fmt"
        "sync"
        "time"

        "github.com/v2e4lisp/sample/singleflight"
)

type User struct {
        Name string
}

func Example() {
        hello := func(v interface{}) (interface{}, error) {
                user, ok := v.(*User)
                if !ok {
                        return nil, errors.New("type error")
                }
                msg := fmt.Sprint("hello: ", user.Name)
                timer := time.NewTimer(time.Second * 1)
                <-timer.C
                fmt.Println("executing")
                return msg, nil
        }
        flight := singleflight.New(hello)

        var wg sync.WaitGroup
        t := func() {
                defer wg.Done()
                user := &User{
                        Name: "wenjun.yan",
                }
                val, err := flight.Do(*user, user)
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
        // hello: wenjun.yan
        // hello: wenjun.yan
        // hello: wenjun.yan
}
