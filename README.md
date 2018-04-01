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

nc
--------

```sh
nc -vz IP_Address Port
```

binary search
---------------

```java
class BSearch {
    public static void main(String[] args) {
        BSearch s = new BSearch();
        int l0 = s.lsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 5);
        int l1 = s.lsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 6);
        int l2 = s.lsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 12);
        int l3 = s.lsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 13);
        int l4 = s.lsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 0);
        int l5 = s.lsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 1);

        int r0 = s.rsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 5);
        int r1 = s.rsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 6);
        int r2 = s.rsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 12);
        int r3 = s.rsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 13);
        int r4 = s.rsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 0);
        int r5 = s.rsearch(new int[]{1,2,3,4,5,5,5,5,9,12}, 1);

        System.out.println("" + l0 + "," + r0);
        System.out.println("" + l1 + "," + r1);
        System.out.println("" + l2 + "," + r2);
        System.out.println("" + l3 + "," + r3);
        System.out.println("" + l4 + "," + r4);
        System.out.println("" + l5 + "," + r5);

        // ➜  tmp java BSearch
        // 4,7
        // 7,8
        // 9,9
        // 9,10
        // -1,0
        // 0,0
    }

    // Return the index of the first target in nums if there is one
    // or the index of the last element which is less than target
    //
    // suppose that nums[-1] = MIN_VALUE and nums[length] == MAX_VALUE;
    // The function guarantees that nums[ret] <= target && nums[ret+1] > target
    public int lsearch(int[] nums, int target) {
        if (nums[0] > target) return -1;
        if (nums[nums.length-1] < target) return nums.length - 1;

        int l = 0;
        int r = nums.length-1;

        // invariant: nums[r] >= target && nums[0..l-1] < target
        while (l < r) {
            int m = (l+r)/2;
            if (nums[m] < target) l = m+1;
            else r = m;
        }

        // now we have:
        // (1) l == r
        // (2) nums[r] >= target && nums[0..l-1] < target;
        //
        // => nums[r] >= target && nums[0, r-1] < target
        if (nums[r] == target) return r;
        return r-1;
    }

    // the counterpart of lsearch
    // Guarantee that either the return value is nums.length or nums[ret] <= target
    public int rsearch(int[] nums, int target) {
        if (nums[0] > target) return 0;
        if (nums[nums.length-1] < target) return nums.length;
        if (nums[nums.length-1] == target) return nums.length - 1;

        int l = 0;
        int r = nums.length-1;

        // invariant: nums[r] > target && nums[0..l-1] <= target
        while (l < r) {
            int m = (l+r)/2;
            if (nums[m] <= target) l = m+1;
            else r = m;
        }

        if (l > 0 && nums[l-1] == target) return l-1;
        return l;
    }
}
```
